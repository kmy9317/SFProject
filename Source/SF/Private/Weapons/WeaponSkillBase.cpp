// Copyright 1998-2024 Epic Games, Inc. All Rights Reserved.

#include "Weapons/WeaponSkillBase.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UWeaponSkillBase::UWeaponSkillBase()
{
	// [멀티플레이] 스킬이 클라이언트에서 예측 실행되고, 서버에서도 실행되도록 설정합니다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

// [핵심 기능 1] 스킬 발동 (메인 엔진)
void UWeaponSkillBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. (검증) MontageToPlay 변수에 몽타주가 잘 연결되어 있는지 확인합니다.
	if (!MontageToPlay)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] ActivateAbility: MontageToPlay is not set!"), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 몽타주 없으면 스킬 캔슬
		return;
	}

	// 2. (자원) CommitAbility()를 호출해 자원 소모를 '확정'합니다.
	// (AttributeSet의 스태미나/FP가 여기서 체크됩니다)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		// 자원이 부족하면 스킬을 종료합니다.
		UE_LOG(LogTemp, Log, TEXT("[%s] ActivateAbility: Failed to commit ability (e.g., not enough stamina)."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

		// --- 3. (실행) 몽타주 '일꾼' 생성 및 실행 ---
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, 
		FName("PlayMontageTask"), 
		MontageToPlay, 
		1.0f,							// Rate
		NAME_None,						// StartSection
		true,							// bStopWhenAbilityEnds
		1.0f,							// AnimRootMotionTranslationScale
		0.0f,							// StartTimeSeconds
		false							// bAllowInterruptAfterBlendOut
	);

	if (MontageTask)
	{
		// '일꾼'에게 '결과 보고'를 받을 콜백 함수 3개를 연결합니다.
		MontageTask->OnCompleted.AddDynamic(this, &UWeaponSkillBase::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UWeaponSkillBase::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UWeaponSkillBase::OnMontageCancelled);

		// '일꾼'에게 작업 시작(Activate) 명령을 내립니다.
		MontageTask->Activate();
	}
	else
	{
		// 태스크 생성에 실패하면 스킬을 종료합니다. (안전장치)
		UE_LOG(LogTemp, Error, TEXT("[%s] ActivateAbility: Failed to create MontageTask."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

// [핵심 기능 2] 스킬 종료 (뒷정리)
void UWeaponSkillBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{ 
	// 몽타주 태스크('일꾼')가 여전히 유효하다면(실행 중이라면) 정리합니다.
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


// --- '결과 보고' 함수들의 실제 내용 ---

// (상황 A: 성공)
void UWeaponSkillBase::OnMontageCompleted()
{
	// 몽타주가 성공적으로 끝까지 재생되었으므로, 스킬을 정상 종료(bWasCancelled=false)합니다.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

// (상황 B: 피격)
void UWeaponSkillBase::OnMontageInterrupted()
{
	// 몽타주가 외부 요인(피격 등)에 의해 중단되었으므로, 스킬을 '캔슬' 상태(bWasCancelled=true)로 종료합니다.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

// (상황 C: 내부 캔슬)
void UWeaponSkillBase::OnMontageCancelled()
{
	// 몽타주가 캔슬되었으므로, 스킬을 '캔슬' 상태(bWasCancelled=true)로 종료합니다.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}