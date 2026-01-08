// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGA_Dragon_TailSwipe.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GameplayEffect/SFGameplayEffectContext.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterBase.h"
#include "DrawDebugHelpers.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"

USFGA_Dragon_TailSwipe::USFGA_Dragon_TailSwipe()
{
	AbilityID = FName("Dragon_TailSwipe");
	AttackType = EAttackType::Melee;
	
	AbilityTags.AddTag(SFGameplayTags::Ability_Dragon_TailSwipe);
	CoolDownTag = SFGameplayTags::Ability_Cooldown_Dragon_TailSwipe;
}

void USFGA_Dragon_TailSwipe::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                             const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	

	// Montage 재생
	if (TailSwipeMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			TailSwipeMontage,
			1.f,
			NAME_None,
			true
		);

		if (MontageTask)
		{
			MontageTask->OnCompleted.AddDynamic(this, &USFGA_Dragon_TailSwipe::OnMontageCompleted);
			MontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_TailSwipe::OnMontageInterrupted);
			MontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_TailSwipe::OnMontageCancelled);
			MontageTask->ReadyForActivation();
		}
	}

	// Authority에서만 히트 이벤트 대기
	if (!ActorInfo->IsNetAuthority())
	{
		return;
	}

	// AnimNotifyState_SweepTrace에서 보내는 GameplayEvent 대기
	WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		SFGameplayTags::GameplayEvent_Tracing,
		nullptr,
		false, 
		true
	);

	if (WaitEventTask)
	{
		WaitEventTask->EventReceived.AddDynamic(this, &USFGA_Dragon_TailSwipe::OnTailHit);
		WaitEventTask->ReadyForActivation();
	}
}



void USFGA_Dragon_TailSwipe::OnTailHit(FGameplayEventData Payload)
{
	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (!Dragon)
	{
		return;
	}


	const FHitResult* HitResult = Payload.ContextHandle.GetHitResult();
	if (!HitResult)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnTailHit: No HitResult in Payload!"));
		return;
	}

	AActor* HitActor = HitResult->GetActor();
	if (!HitActor)
	{
		return;
	}

	if (GetAttitudeTowards(HitActor) != ETeamAttitude::Hostile)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = MakeEffectContext(CurrentSpecHandle, CurrentActorInfo);
	EffectContext.AddHitResult(*HitResult);

	ApplyDamageToTarget(HitActor, EffectContext);

	ApplyKnockBackToTarget(HitActor, HitResult->ImpactPoint);

	// Pressure 적용 
	ApplyPressureToTarget(HitActor);
}

void USFGA_Dragon_TailSwipe::TailLaunchToTarget(AActor* Target, const FVector& HitLocation)
{
	if (!Target)
	{
		return;
	}

	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (!Dragon)
	{
		return;
	}
	
	FVector ToTarget = HitLocation - Dragon->GetActorLocation();
	ToTarget.Z = 0.f;
	ToTarget.Normalize();
	
	float SideSign = FVector::DotProduct(ToTarget,Dragon->GetActorRightVector()) >= 0.f ? 1.f : -1.f;

	FVector HorizontalDir = Dragon->GetActorRightVector() * SideSign;
	HorizontalDir.Normalize();

	const float VerticalBias = 0.6f;

	FVector DiagonalDir = (HorizontalDir + FVector::UpVector * VerticalBias).GetSafeNormal();

	ApplyLaunchToTarget(Target, DiagonalDir, 1.0f);


}

void USFGA_Dragon_TailSwipe::OnMontageCompleted()
{

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Dragon_TailSwipe::OnMontageInterrupted()
{

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Dragon_TailSwipe::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Dragon_TailSwipe::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (WaitEventTask)
	{
		WaitEventTask->EndTask();
		WaitEventTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


float USFGA_Dragon_TailSwipe::CalcScoreModifier(const FEnemyAbilitySelectContext& Context) const
{
	float Modifier = 0.f; 

	const FBossEnemyAbilitySelectContext* BossContext =
	   static_cast<const FBossEnemyAbilitySelectContext*>(&Context);

	
	if (BossContext && BossContext->Zone == EBossAttackZone::Melee)
	{
		Modifier += 800.f;
	}
	else
	{
		return -1000.f; 
	}

	if (!Context.Target) return Modifier;
	
	if (Context.AngleToTarget < 60.f) // 전방 60도 이내
	{
		Modifier += 400.f;
	}
	else if (Context.AngleToTarget > 120.f) // 내 뒤에 있음
	{
		Modifier -= 1000.f; 
	}

	UAbilitySystemComponent* TargetASC =
	   UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Context.Target);

	if (!TargetASC) return Modifier;
	
	if (TargetASC->HasMatchingGameplayTag(SFGameplayTags::Dragon_Pressure_Forward))
	{
		Modifier += 500.f; // 달라붙은 적을 떼어놓기 위해 우선순위 급상승
	}
	
	if (TargetASC->HasMatchingGameplayTag(SFGameplayTags::Dragon_Pressure_Back))
	{
		Modifier -= 300.f;
	}

	return Modifier;
}
