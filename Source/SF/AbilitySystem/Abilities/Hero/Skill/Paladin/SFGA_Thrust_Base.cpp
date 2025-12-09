#include "SFGA_Thrust_Base.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFHeroSkillTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"

USFGA_Thrust_Base::USFGA_Thrust_Base(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilityTags.AddTag(SFGameplayTags::Ability_Skill_Primary_Hero);
}

void USFGA_Thrust_Base::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ASFCharacterBase* SFCharacter = GetSFCharacterFromActorInfo();
	if (SFCharacter == nullptr || SFCharacter->GetCharacterMovement()->IsFalling())
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	if (ThrustMontage)
	{
		if (UAbilityTask_PlayMontageAndWait* ThrustMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("ThrustMontage"), ThrustMontage, 1.f, NAME_None, true))
		{
			ThrustMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
			ThrustMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
			ThrustMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
			ThrustMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageFinished);
			ThrustMontageTask->ReadyForActivation();
		}
	}

	if (UAbilityTask_WaitGameplayEvent* TraceEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SFGameplayTags::GameplayEvent_Tracing, nullptr, false, true))
	{
		TraceEventTask->EventReceived.AddDynamic(this, &ThisClass::OnTrace);
		TraceEventTask->ReadyForActivation();
	}

	if (UAbilityTask_WaitGameplayEvent* ThrustBeginEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SFGameplayTags::GameplayEvent_Montage_Begin, nullptr, true, true))
	{
		ThrustBeginEventTask->EventReceived.AddDynamic(this, &ThisClass::OnThrustBegin);
		ThrustBeginEventTask->ReadyForActivation();
	}
	if (UAbilityTask_WaitGameplayEvent* ThrustEndEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SFGameplayTags::GameplayEvent_Montage_End, nullptr, true, true))
	{
		ThrustEndEventTask->EventReceived.AddDynamic(this, &ThisClass::OnThrustEnd);
		ThrustEndEventTask->ReadyForActivation();
	}
}

void USFGA_Thrust_Base::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGA_Thrust_Base::OnThrustBegin(FGameplayEventData Payload)
{
	// TODO : 스킬 시작시 움직임, 카메라 rotation yaw 설정등 필요한 작업 수행
}

void USFGA_Thrust_Base::OnThrustEnd(FGameplayEventData Payload)
{
	// TODO : 스킬 종료시 변경된 설정 기본값으로 되돌리기
}

void USFGA_Thrust_Base::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
