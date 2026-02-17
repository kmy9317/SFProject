#include "SFGA_AreaOverTimeSkill.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Libraries/SFCombatLibrary.h"

USFGA_AreaOverTimeSkill::USFGA_AreaOverTimeSkill()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	DamageSetByCallerTag = SFGameplayTags::Data_Damage_BaseDamage;
	BaseDamage.Value = 10.0f;
}

void USFGA_AreaOverTimeSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bMontageFinished = false;
	bDurationExpired = false;
	
	PlaySkillAnimation();
}

void USFGA_AreaOverTimeSkill::PlaySkillAnimation()
{
	if (!SkillMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SkillMontage, 1.0f, NAME_None, false, 1.0f);
	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCompleted);
		MontageTask->ReadyForActivation();
	}

	if (TriggerEventTag.IsValid())
	{
		WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TriggerEventTag);
		if (WaitEventTask)
		{
			WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnGameplayEventReceived);
			WaitEventTask->ReadyForActivation();
		}
	}
	else
	{
		StartAttackLoop();
	}
}

void USFGA_AreaOverTimeSkill::OnMontageCompleted()
{
	bMontageFinished = true;
	TryFinishAbility();
}

void USFGA_AreaOverTimeSkill::OnGameplayEventReceived(FGameplayEventData Payload)
{
	StartAttackLoop();
}

void USFGA_AreaOverTimeSkill::StartAttackLoop()
{
	if (LoopingGameplayCueTag.IsValid())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			FGameplayCueParameters CueParams;
			CueParams.EffectCauser = GetAvatarActorFromActorInfo();
			CueParams.Location = GetAvatarActorFromActorInfo()->GetActorLocation();
			ASC->AddGameplayCue(LoopingGameplayCueTag, CueParams);
		}
	}
	
	PerformAreaTick();
	
	GetWorld()->GetTimerManager().SetTimer(LoopTimerHandle,this,&ThisClass::PerformAreaTick,DamageInterval,true);
	GetWorld()->GetTimerManager().SetTimer(DurationTimerHandle,this,&ThisClass::OnDurationExpired,AttackDuration,false);
}

void USFGA_AreaOverTimeSkill::OnDurationExpired()
{
	GetWorld()->GetTimerManager().ClearTimer(LoopTimerHandle);
	
	bDurationExpired = true;
	TryFinishAbility();
}

void USFGA_AreaOverTimeSkill::TryFinishAbility()
{
	// 몽타주와 지속시간 모두 끝나야 종료
	if (bMontageFinished && bDurationExpired)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void USFGA_AreaOverTimeSkill::PerformAreaTick()
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}
	
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!AvatarActor || !SourceASC)
	{
		return;
	}

	float DamageAmount = BaseDamage.GetValueAtLevel(GetAbilityLevel());
	FSFAreaDamageParams Params;
	Params.SourceASC = SourceASC;
	Params.SourceActor = AvatarActor;
	Params.EffectCauser = AvatarActor;
	Params.Origin = AvatarActor->GetActorLocation();
	Params.OverlapShape = FCollisionShape::MakeSphere(Radius);
	Params.DamageAmount = DamageAmount;
	Params.DamageSetByCallerTag = DamageSetByCallerTag;
	Params.DamageGEClass = DamageGameplayEffectClass;
	Params.DebuffGEClass = DebuffGameplayEffectClass;

	// 추가 디버프 GE가 설정되어 있으면 추가 효과로 포함
	if (bApplyConditionalCC && ConditionalCCEffectClass)
	{
		Params.AdditionalEffects.Add(ConditionalCCEffectClass);
	}

	USFCombatLibrary::ApplyAreaDamage(Params);
}

void USFGA_AreaOverTimeSkill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LoopTimerHandle);
		World->GetTimerManager().ClearTimer(DurationTimerHandle);
	}

	if (LoopingGameplayCueTag.IsValid())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			ASC->RemoveGameplayCue(LoopingGameplayCueTag);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}