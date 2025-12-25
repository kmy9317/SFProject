#include "SFPlayerCombatStateComponent.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/SFCharacterGameplayTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Messages/SFMessageGameplayTags.h"
#include "Messages/SFPortalInfoMessages.h"
#include "Net/UnrealNetwork.h"
#include "Player/SFPlayerState.h"

USFPlayerCombatStateComponent::USFPlayerCombatStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

USFPlayerCombatStateComponent* USFPlayerCombatStateComponent::FindPlayerCombatStateComponent(const AActor* Actor)
{
	// PlayerState에서 직접 찾기
	if (USFPlayerCombatStateComponent* CombatStateComponent = Actor ? Actor->FindComponentByClass<USFPlayerCombatStateComponent>() : nullptr)
	{
		return CombatStateComponent;
	}

	// Pawn인 경우 PlayerState에서 찾기
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (const APlayerState* PS = Pawn->GetPlayerState())
		{
			return PS->FindComponentByClass<USFPlayerCombatStateComponent>();
		}
	}

	return nullptr;
}

void USFPlayerCombatStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CombatInfo);
}

void USFPlayerCombatStateComponent::BeginPlay()
{
	Super::BeginPlay();
	
	InitialDownCount = InitialReviveGaugeByDownCount.Num();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		CombatInfo.RemainingDownCount = InitialDownCount;
		CachedCombatInfo = CombatInfo;
	}
}

float USFPlayerCombatStateComponent::GetInitialReviveGauge() const
{
	// 현재 사용할 인덱스 = 총 다운 횟수 - 남은 횟수
	const int32 DownIndex = InitialDownCount - CombatInfo.RemainingDownCount;
	
	if (InitialReviveGaugeByDownCount.IsValidIndex(DownIndex))
	{
		return InitialReviveGaugeByDownCount[DownIndex];
	}

	// 배열 범위 초과 시 0 (즉시 사망)
	return 0.f;
}

bool USFPlayerCombatStateComponent::IsDowned() const
{
	// ASC에서 Downed 태그 체크
	if (AActor* Owner = GetOwner())
	{
		if (ASFPlayerState* PS = Cast<ASFPlayerState>(Owner))
		{
			if (USFAbilitySystemComponent* ASC = PS->GetSFAbilitySystemComponent())
			{
				return ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Downed);
			}
		}
	}
	return false;
}

void USFPlayerCombatStateComponent::SetIsDead(bool bNewIsDead)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    if (CombatInfo.bIsDead != bNewIsDead)
    {
        CombatInfo.bIsDead = bNewIsDead;

        BroadcastCombatInfoChanged();
        BroadcastDeadStateChanged();
    }
}

void USFPlayerCombatStateComponent::DecrementDownCount()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (CombatInfo.RemainingDownCount > 0)
	{
		CombatInfo.RemainingDownCount--;
		BroadcastCombatInfoChanged();
	}
}

void USFPlayerCombatStateComponent::ResetDownCount()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (CombatInfo.RemainingDownCount != InitialDownCount)
	{
		CombatInfo.RemainingDownCount = InitialDownCount;
		BroadcastCombatInfoChanged();
	}
}

void USFPlayerCombatStateComponent::IncrementReviveCount()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	CombatInfo.ReviveCount++;
	BroadcastCombatInfoChanged();
}

void USFPlayerCombatStateComponent::OnRep_CombatInfo()
{
	bHasReceivedInitialCombatInfo = true;
	
	const bool bDeadChanged = (CachedCombatInfo.bIsDead != CombatInfo.bIsDead);

	BroadcastCombatInfoChanged();
	// Dead 상태 변경 감지에 따라 추가적인 Message 전달 처리
	if (bDeadChanged)
	{
		BroadcastDeadStateChanged();
	}
}

void USFPlayerCombatStateComponent::BroadcastCombatInfoChanged()
{
	if (CachedCombatInfo != CombatInfo)
	{
		CachedCombatInfo = CombatInfo;
		OnCombatInfoChanged.Broadcast(CombatInfo);
	}
}

void USFPlayerCombatStateComponent::SetCombatInfoFromTravel(const FSFHeroCombatInfo& InCombatInfo)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const bool bWasDead = CombatInfo.bIsDead;
	
	CombatInfo = InCombatInfo;
	CachedCombatInfo = CombatInfo;

	if (CombatInfo.bIsDead != bWasDead)
	{
		BroadcastDeadStateChanged();
	}
}

bool USFPlayerCombatStateComponent::HasReceivedInitialCombatInfo() const
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return true;
	}
    
	return bHasReceivedInitialCombatInfo;
}

void USFPlayerCombatStateComponent::MarkInitialDataReceived()
{
	if (!bHasReceivedInitialCombatInfo)
	{
		bHasReceivedInitialCombatInfo = true;
		BroadcastCombatInfoChanged();
	}
}

void USFPlayerCombatStateComponent::BroadcastDeadStateChanged()
{
	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		FSFPlayerDeadStateMessage Message;
		Message.PlayerState = Cast<APlayerState>(GetOwner());
		Message.bIsDead = CombatInfo.bIsDead;
		MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Player_DeadStateChanged, Message);
	}
}

#if WITH_EDITOR
void USFPlayerCombatStateComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 변경된 속성이 있는지 확인
	if (PropertyChangedEvent.Property)
	{
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();

		// 배열(InitialReviveGaugeByDownCount)변경 확인
		if (PropertyName == GET_MEMBER_NAME_CHECKED(USFPlayerCombatStateComponent, InitialReviveGaugeByDownCount))
		{
			// InitialDownCount를 배열 크기로 업데이트
			InitialDownCount = InitialReviveGaugeByDownCount.Num();
		}
	}
}
#endif
