#include "SFGA_Hero_Death.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Animation/SFAnimationGameplayTags.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/Hero/SFHero.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "Player/SFPlayerController.h"
#include "Player/Components/SFPlayerCombatStateComponent.h"


USFGA_Hero_Death::USFGA_Hero_Death(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = ESFAbilityActivationPolicy::Manual;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	AbilityTags.AddTag(SFGameplayTags::Ability_Hero_Death);

	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Dead);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Dead);

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = SFGameplayTags::GameplayEvent_Death;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void USFGA_Hero_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (HasAuthority(&CurrentActivationInfo))
	{
		if (USFPlayerCombatStateComponent* CombatState = USFPlayerCombatStateComponent::FindPlayerCombatStateComponent(GetAvatarActorFromActorInfo()))
		{
			CombatState->SetIsDead(true);
		}
	}

	CancelAllActiveAbilities();
	HideWeapons();
	DisablePlayerInput();
	PlayDeathMontage();
}

void USFGA_Hero_Death::CancelAllActiveAbilities()
{
	if (USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo())
	{
		// Downed 취소 (Downed → Death 경로)
		FGameplayTagContainer DownedTag;
		DownedTag.AddTag(SFGameplayTags::Character_State_Downed);
		ASC->CancelAbilities(&DownedTag);

		// 나머지 어빌리티 취소 
		ASC->CancelActiveAbilities(nullptr, nullptr, this, true);
	}
}

void USFGA_Hero_Death::HideWeapons()
{
	if (USFEquipmentComponent* EquipmentComp = GetEquipmentComponent())
	{
		EquipmentComp->HideWeapons();
	}
}

void USFGA_Hero_Death::DisablePlayerInput()
{
	if (ASFHero* Hero = Cast<ASFHero>(GetAvatarActorFromActorInfo()))
	{
		if (UCharacterMovementComponent* MovementComp = Hero->GetCharacterMovement())
		{
			MovementComp->StopMovementImmediately();
			MovementComp->DisableMovement();
		}
	}

	if (ASFPlayerController* PC = GetSFPlayerControllerFromActorInfo())
	{
		PC->SetIgnoreMoveInput(true);
	}

	// ASC 입력 버퍼 클리어
	if (IsLocallyControlled())
	{
		if (USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo())
		{
			ASC->ClearAbilityInput();
		}
	}
}

void USFGA_Hero_Death::RestorePlayerInput()
{
	if (ASFHero* Hero = Cast<ASFHero>(GetAvatarActorFromActorInfo()))
	{
		if (UCharacterMovementComponent* MovementComp = Hero->GetCharacterMovement())
		{
			MovementComp->SetMovementMode(MOVE_Walking);
		}
	}

	if (ASFPlayerController* PC = GetSFPlayerControllerFromActorInfo())
	{
		PC->SetIgnoreMoveInput(false);
	}
}

void USFGA_Hero_Death::PlayDeathMontage()
{
	if (const USFHeroAnimationData* AnimData = GetHeroAnimationData())
	{
		FSFMontagePlayData MontageData = AnimData->GetSingleMontage(SFGameplayTags::Montage_State_Death);
		if (MontageData.IsValid() && MontageData.Montage)
		{
			if (UAbilityTask_PlayMontageAndWait* DeathMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, 
				TEXT("DeathMontage"), 
				MontageData.Montage, 
				MontageData.PlayRate, 
				MontageData.StartSection, 
				false,  // bStopWhenAbilityEnds = false
				1.f, 
				0.f, 
				false))
			{
				DeathMontageTask->ReadyForActivation();
				return;
			}
		}
	}

	HandlePostDeath();
}

void USFGA_Hero_Death::HandlePostDeath()
{
	// TODO : 사망 후 처리 (사망 UI Fade in/out, 관전 모드 등)
}

void USFGA_Hero_Death::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled)
	{
		RestorePlayerInput();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}