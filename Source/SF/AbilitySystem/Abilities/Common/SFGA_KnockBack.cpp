#include "SFGA_KnockBack.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"

USFGA_KnockBack::USFGA_KnockBack(const FObjectInitializer& ObjectInitializer)
{
	ActivationPolicy = ESFAbilityActivationPolicy::Manual;

	// 연속으로 넉백을 맞을 경우 새로운 넉백으로 덮어씀
	bRetriggerInstancedAbility = true;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnlyTermination;

	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Knockback);

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = SFGameplayTags::GameplayEvent_Knockback;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void USFGA_KnockBack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (TriggerEventData == nullptr || TriggerEventData->Instigator == nullptr)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	// TODO : 추후 공격자 전방이 아닌 전 방향 넉백 구현 고려(적절한 공격자의 애니메이이션 존재시)
	FVector WorldDirection = TriggerEventData->Instigator->GetActorForwardVector();
	if (UAbilityTask_ApplyRootMotionConstantForce* RootMotionForceTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this,
		TEXT("KnockbackForce"),
		WorldDirection,           
		KnockbackStrength, 
		KnockbackDuration,
		true, // bIsAdditive: 기존 속도에 더함
		KnockbackStrengthCurve, // 시간에 따른 강도 커브
		ERootMotionFinishVelocityMode::ClampVelocity, // 종료 시 속도 처리
		FVector::ZeroVector, // 종료 시 속도 설정값
		100.f, // 속도 클램프 값
		true)) // bEnableGravity: 중력 적용
	{
		// 넉백 완료 시 OnKnockbackFinished 콜백 호출
		RootMotionForceTask->OnFinish.AddDynamic(this, &ThisClass::OnKnockbackFinished);
		RootMotionForceTask->ReadyForActivation();
	}

	// 공격자와 타겟의 상대적 각도에 따라 적절한 방향의 몽타주 선택
	UAnimMontage* SelectedMontage = SelectDirectionalMontage(TriggerEventData->Instigator, GetAvatarActorFromActorInfo());

	if (SelectedMontage)
	{
		if (UAbilityTask_PlayMontageAndWait* KnockbackMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("KnockbackMontage"), SelectedMontage, 1.f, NAME_None, true))
		{
			// 어빌리티 종료는 RootMotion 완료후 처리
			KnockbackMontageTask->ReadyForActivation();
		}
	}
}

UAnimMontage* USFGA_KnockBack::SelectDirectionalMontage(const AActor* Source, const AActor* Target) const
{
	if (Source == nullptr || Target == nullptr)
	{
		return nullptr;
	}

	// 공격자와 타겟의 전방 벡터를 Rotator로 변환
	const FRotator& SourceRotator = UKismetMathLibrary::Conv_VectorToRotator(Source->GetActorForwardVector());
	const FRotator& TargetRotator = UKismetMathLibrary::Conv_VectorToRotator(Target->GetActorForwardVector());

	// 두 Rotator의 정규화된 차이 계산 (Yaw: -180 ~ 180)
	const FRotator& DeltaRotator = UKismetMathLibrary::NormalizedDeltaRotator(SourceRotator, TargetRotator);
	float YawAbs = FMath::Abs(DeltaRotator.Yaw);

	UAnimMontage* SelectedMontage;

	// 방향 판정 및 몽타주 선택
	if (YawAbs < ForwardThreshold)
	{
		// 같은 방향 → 뒤에서 공격 → 타겟이 앞으로 밀림
		SelectedMontage = KnockbackForwardMontage;
	}
	else if (YawAbs > BackwardThreshold)
	{
		// 반대 방향 → 앞에서 공격 → 타겟이 뒤로 밀림
		SelectedMontage = KnockbackBackwardMontage;
	}
	else if (DeltaRotator.Yaw < 0.f)
	{
		// 오른쪽에서 공격 → 타겟이 왼쪽으로 밀림
		SelectedMontage = KnockbackLeftMontage;
	}
	else
	{
		// 왼쪽에서 공격 → 타겟이 오른쪽으로 밀림
		SelectedMontage = KnockbackRightMontage;
	}

	return SelectedMontage;
}

void USFGA_KnockBack::OnKnockbackFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_KnockBack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


