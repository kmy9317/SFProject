#include "SFGA_Interact_Chest.h"

#include "Actors/SFChestBase.h"

USFGA_Interact_Chest::USFGA_Interact_Chest(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

void USFGA_Interact_Chest::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (TriggerEventData == nullptr || bInitialized == false)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	ASFChestBase* ChestActor = Cast<ASFChestBase>(InteractableActor);
	if (ChestActor == nullptr)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	ChestActor->SetChestState(ESFChestState::Open);

	if (IsLocallyControlled())
	{
		// TODO : 강화 선택지 위젯 생성
	}

	// TODO : 위젯 종료시 or 상호작용 하자마자 둘 중 선택 고민중
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Interact_Chest::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsLocallyControlled() && ActivateWidget)
	{
		// TODO : 위젯 정리
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

