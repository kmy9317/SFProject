// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGA_Interact_Portal.h"

#include "Actors/SFPortal.h"

USFGA_Interact_Portal::USFGA_Interact_Portal(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USFGA_Interact_Portal::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!bInitialized)
	{
		return;
	}

	ASFPortal* Portal = Cast<ASFPortal>(InteractableActor);
	if (!Portal)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	// Ready 토글 요청
	if (AController* Controller = GetControllerFromActorInfo())
	{
		if (APlayerState* PlayerState = Controller->PlayerState)
		{
			Portal->TogglePlayerReady(PlayerState);
		}
	}

	// 즉시 종료
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

