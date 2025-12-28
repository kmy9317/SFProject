#include "SFSpectatorHUDWidget.h"

#include "Components/TextBlock.h"
#include "Player/SFPlayerState.h"
#include "Player/Components/SFSpectatorComponent.h"

void USFSpectatorHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (USFSpectatorComponent* SpectatorComp = GetSpectatorComponent())
	{
		SpectatorComp->OnSpectatorTargetChanged.AddDynamic(this, &ThisClass::OnSpectatorTargetChanged);

		// 이미 관전 중이면 초기값 표시
		if (APawn* CurrentTarget = SpectatorComp->GetCurrentSpectatorTarget())
		{
			OnSpectatorTargetChanged(CurrentTarget);
		}
	}
}

void USFSpectatorHUDWidget::NativeDestruct()
{
	UnbindFromCurrentTarget();
	
	if (USFSpectatorComponent* SpectatorComp = CachedSpectatorComponent.Get())
	{
		SpectatorComp->OnSpectatorTargetChanged.RemoveDynamic(this, &ThisClass::OnSpectatorTargetChanged);
	}
	
	Super::NativeDestruct();
}

void USFSpectatorHUDWidget::OnSpectatorTargetChanged(APawn* NewTarget)
{
	// 이전 타겟 언바인드
	UnbindFromCurrentTarget();
	
	if (!Text_SpectatingPlayerName)
	{
		return;
	}

	if (!NewTarget)
	{
		Text_SpectatingPlayerName->SetText(FText::FromString(TEXT("No players alive")));
		return;
	}

	ASFPlayerState* SFPS = NewTarget->GetPlayerState<ASFPlayerState>();
	if (!SFPS)
	{
		Text_SpectatingPlayerName->SetText(FText::FromString(TEXT("Unknown")));
		return;
	}

	// 새 타겟 캐시 및 델리게이트 바인딩
	CachedTargetPlayerState = SFPS;
	SFPS->OnPlayerInfoChanged.AddDynamic(this, &ThisClass::OnTargetPlayerInfoChanged);

	// 현재 값으로 업데이트
	UpdateTargetName(SFPS);
}

void USFSpectatorHUDWidget::OnTargetPlayerInfoChanged(const FSFPlayerSelectionInfo& NewPlayerSelection)
{
	if (!Text_SpectatingPlayerName)
	{
		return;
	}

	const FString& Nickname = NewPlayerSelection.GetPlayerNickname();
	if (!Nickname.IsEmpty())
	{
		Text_SpectatingPlayerName->SetText(FText::FromString(Nickname));
	}
}

void USFSpectatorHUDWidget::UpdateTargetName(ASFPlayerState* SFPS)
{
	if (!SFPS || !Text_SpectatingPlayerName)
	{
		return;
	}

	const FString& Nickname = SFPS->GetPlayerSelection().GetPlayerNickname();
    
	if (!Nickname.IsEmpty())
	{
		// PlayerNickname이 있으면 사용
		Text_SpectatingPlayerName->SetText(FText::FromString(Nickname));
	}
	else
	{
		// Fallback: 아직 복제 안 됐으면 PlayerState의 기본 이름 사용
		Text_SpectatingPlayerName->SetText(FText::FromString(SFPS->GetPlayerName()));
	}
}

void USFSpectatorHUDWidget::UnbindFromCurrentTarget()
{
	if (ASFPlayerState* SFPS = CachedTargetPlayerState.Get())
	{
		SFPS->OnPlayerInfoChanged.RemoveDynamic(this, &ThisClass::OnTargetPlayerInfoChanged);
	}
	CachedTargetPlayerState.Reset();
}

USFSpectatorComponent* USFSpectatorHUDWidget::GetSpectatorComponent() const
{
	if (CachedSpectatorComponent.IsValid())
	{
		return CachedSpectatorComponent.Get();
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		USFSpectatorComponent* SpectatorComp = PC->FindComponentByClass<USFSpectatorComponent>();
		CachedSpectatorComponent = SpectatorComp;
		return SpectatorComp;
	}

	return nullptr;
}

