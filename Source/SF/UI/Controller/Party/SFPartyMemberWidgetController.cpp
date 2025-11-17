#include "SFPartyMemberWidgetController.h"

#include "SFLogChannels.h"
#include "AbilitySystem/Attributes/Hero/SFPrimarySet_Hero.h"
#include "Player/SFPlayerState.h"

void USFPartyMemberWidgetController::BroadcastInitialSets()
{
	ASFPlayerState* SFPS = Cast<ASFPlayerState>(TargetPlayerState);
	if (SFPS)
	{
		// 1. 초기 어트리뷰트 브로드캐스트
		OnHealthChanged.Broadcast(TargetPrimarySet->GetHealth());
		OnMaxHealthChanged.Broadcast(TargetPrimarySet->GetMaxHealth());
		
		// 추가적으로 필요한 Attribute 브로드 캐스트

		// 2. 초기 플레이어 정보 브로드캐스트
		OnPlayerInfoChanged.Broadcast(SFPS->GetPlayerSelection());
	}
}

void USFPartyMemberWidgetController::BindCallbacksToDependencies()
{
	ASFPlayerState* SFPS = Cast<ASFPlayerState>(TargetPlayerState);
	if (!SFPS ||! TargetAbilitySystemComponent ||! TargetPrimarySet)
	{
		UE_LOG(LogSF, Warning, TEXT("USFPartyMemberWidgetController: Invalid PlayerState or ASC."));
		return;
	}

	TWeakObjectPtr<USFPartyMemberWidgetController> WeakThis(this);
	if (TargetPrimarySet && TargetAbilitySystemComponent)
	{
		TargetAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(TargetPrimarySet->GetHealthAttribute()).AddLambda(
			[WeakThis](const FOnAttributeChangeData& Data)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnHealthChanged.Broadcast(Data.NewValue);
			}
		});
		TargetAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(TargetPrimarySet->GetMaxHealthAttribute()).AddLambda(
			[WeakThis](const FOnAttributeChangeData& Data)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnMaxHealthChanged.Broadcast(Data.NewValue);
			}
		});
	}
	
	// (필요시 나머지 어트리뷰트 바인딩)

	// PlayerState의 Info 델리게이트에 바인딩
	SFPS->OnPlayerInfoChanged.AddDynamic(this, &ThisClass::HandlePlayerInfoChanged);
}

void USFPartyMemberWidgetController::HandlePlayerInfoChanged(const FSFPlayerSelectionInfo& NewPlayerSelection)
{
	OnPlayerInfoChanged.Broadcast(NewPlayerSelection);
}