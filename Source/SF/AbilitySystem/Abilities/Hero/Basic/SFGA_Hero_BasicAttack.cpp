#include "SFGA_Hero_BasicAttack.h"

// Engine & Ability System Includes
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"

// Framework Includes
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h" 

// Project Includes
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AbilitySystem/Attributes/Hero/SFPrimarySet_Hero.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/Hero/Component/SFLockOnComponent.h"
#include "Input/SFInputGameplayTags.h"
#include "SFBasicAttackData.h"

USFGA_Hero_BasicAttack::USFGA_Hero_BasicAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	
	ComboWindowTag = SFGameplayTags::Character_State_ComboWindow;
	
	// [중요] 부모 클래스(SFGA_Skill_Melee)의 자동 워핑 Task를 비활성화함
	// 자식 클래스에서 독자적인 원거리 투영(Far Projection) 방식을 사용하기 위함임
	bUseWindupWarp = false; 
	
	bUseEquipmentWarpSettings = false; 
	
	WarpTargetNameOverride = FName("AttackTarget");
	WarpRangeOverride = 500.f;          // 안전장치 거리값임
	RotationInterpSpeedOverride = 20.f; // 반응성을 위해 회전 속도를 20으로 설정함
}

// ----------------------------------------------------------------------------------------------------------------
// Lifecycle Overrides
// ----------------------------------------------------------------------------------------------------------------

void USFGA_Hero_BasicAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 데이터 에셋 로드 및 검증을 수행함
	if (AttackDataAsset) AttackSteps = AttackDataAsset->AttackSteps;
	if (AttackSteps.Num() == 0) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	// 부모 클래스 호출 (bUseWindupWarp가 false이므로 부모의 워핑 Task는 실행되지 않음)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 첫 공격 시, 현재 캐릭터의 입력 방향 혹은 정면 방향으로 캐시를 초기화함
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		FVector MoveInput = Character->GetLastMovementInputVector();
		CachedTargetRotation = MoveInput.IsNearlyZero() ? Character->GetActorRotation() : MoveInput.Rotation();
	}

	// 히트 판정(Trace) 이벤트를 대기함
	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SFGameplayTags::GameplayEvent_Tracing);
	if (HitTask)
	{
		HitTask->EventReceived.AddDynamic(this, &ThisClass::OnTrace);
		HitTask->ReadyForActivation();
	}

	// 첫 번째 공격 단계를 실행함
	ExecuteAttackStep(CurrentStepIndex);
}

void USFGA_Hero_BasicAttack::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	// 차징(Charging) 로직을 처리함
	if (bIsCharging)
	{
		float ChargeDuration = GetWorld()->GetTimeSeconds() - ChargeStartTime;
		bIsCharging = false;

		if (AttackSteps.IsValidIndex(CurrentStepIndex))
		{
			const FSFBasicAttackStep& CurrentStep = AttackSteps[CurrentStepIndex];
			
			// 최소 차징 시간을 충족했을 경우 다음 단계로 진행함
			if (ChargeDuration >= CurrentStep.MinChargeTime)
			{
				if (ActiveMontageTask)
				{
					ActiveMontageTask->EndTask(); 
					ActiveMontageTask = nullptr;
				}

				CurrentStepIndex++; 
				ExecuteAttackStep(CurrentStepIndex);
			}
			else
			{
				// 차징 실패 시 어빌리티를 종료함
				EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			}
		}
	}
}

void USFGA_Hero_BasicAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 상태 변수를 초기화함
	CurrentStepIndex = 0;
	bInputReserved = false;
	bIsCharging = false;
	ChargeStartTime = 0.0f;

	RemoveStepGameplayTags();
	RestoreSlidingMode();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ----------------------------------------------------------------------------------------------------------------
// Core Execution
// ----------------------------------------------------------------------------------------------------------------

void USFGA_Hero_BasicAttack::ExecuteAttackStep(int32 StepIndex)
{
	if (!AttackSteps.IsValidIndex(StepIndex))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	
	// [수정됨] 고정 비용(CommitAbilityCost) 대신 단계별 동적 비용을 적용함
	// if (!CommitAbilityCost(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	// {
	// 	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	// 	return;
	// }

	const FSFBasicAttackStep& CurrentStep = AttackSteps[StepIndex];
	
	ResetHitActors();

	// 단계별 스태미나 비용을 지불하고, 부족하면 어빌리티를 종료함
	if (!CheckAndApplyStepCost(CurrentStep.StaminaCost))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 콤보 선입력을 위한 Input Press 대기 Task를 활성화함
	UAbilityTask_WaitInputPress* InputTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
	if (InputTask)
	{
		InputTask->OnPress.AddDynamic(this, &ThisClass::OnInputPressed);
		InputTask->ReadyForActivation();
	}

	RemoveStepGameplayTags();
	
	CurrentDamageMultiplier = CurrentStep.DamageMultiplier;

	if (CurrentStep.ChainSlidingMode != ESFSlidingMode::None)
	{
		ApplySlidingMode(CurrentStep.ChainSlidingMode);
	}

	// [중요] 입력 캐싱 및 원거리 투영을 적용하여 워핑 타겟을 갱신함
	UpdateWarpTargetFromInput();

	ApplyStepGameplayTags(CurrentStep.TempAbilityTags);

	// 콤보 윈도우 태그 제거(입력 시간 만료)를 감지하는 Task를 활성화함
	UAbilityTask_WaitGameplayTagRemoved* TagRemovedTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(this, ComboWindowTag);
	if (TagRemovedTask)
	{
		TagRemovedTask->Removed.AddDynamic(this, &ThisClass::OnComboWindowTagRemoved);
		TagRemovedTask->ReadyForActivation();
	}

	// 차징 단계일 경우 시작 시간을 기록함
	if (CurrentStep.bIsChargeStep)
	{
		bIsCharging = true;
		ChargeStartTime = GetWorld()->GetTimeSeconds();
	}
	
	// 몽타주를 재생함
	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, CurrentStep.Montage, 1.0f, NAME_None, true, 1.0f, 0.1f, false);
	
	if (ActiveMontageTask)
	{
		ActiveMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
		ActiveMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
		ActiveMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
		ActiveMontageTask->ReadyForActivation();
	}
}

// ----------------------------------------------------------------------------------------------------------------
// Motion Warping & Logic
// ----------------------------------------------------------------------------------------------------------------

void USFGA_Hero_BasicAttack::UpdateWarpTargetFromInput()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character) return;

	// 1. 락온 여부 확인
	bool bIsLockedOn = false;
	if (USFLockOnComponent* LockOnComp = Character->FindComponentByClass<USFLockOnComponent>())
	{
		if (LockOnComp->GetCurrentTarget())
		{
			bIsLockedOn = true;
		}
	}

	// [분기 처리]
	if (bIsLockedOn)
	{
		// Case A: 락온 상태 -> 부모 클래스의 로직 사용 (락온 타겟 우선 + 지터링 방지)
		UpdateWarpTargetImmediately();
	}
	else
	{
		// Case B: 락온 아님 -> BasicAttack 고유의 "캐시된 입력(CachedTargetRotation)" 사용
		UMotionWarpingComponent* MotionWarpingComp = Character->GetComponentByClass<UMotionWarpingComponent>();
		if (MotionWarpingComp)
		{
			FRotator TargetRotation = CachedTargetRotation;
			TargetRotation.Pitch = 0.f;
			TargetRotation.Roll = 0.f;

			// Far Projection (캐릭터 회전 방지)
			const float DirectionalWarpDistance = 10000.f; 
			FVector TargetLocation = Character->GetActorLocation() + (TargetRotation.Vector() * DirectionalWarpDistance);

			// 워핑 적용
			MotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetNameOverride, TargetLocation, TargetRotation);

			// 서버 동기화 (부모 클래스의 RPC 재사용)
			if (Character->IsLocallyControlled() && !Character->HasAuthority())
			{
				ServerSetWarpRotation(WarpTargetNameOverride, TargetRotation);
			}
		}
	}
}

bool USFGA_Hero_BasicAttack::CheckAndApplyStepCost(float CostAmount)
{
	// 비용이 0 이하이면 성공으로 간주함
	if (CostAmount <= 0.f) return true;
	// 설정된 비용 GE가 없으면 검사 없이 통과시킴
	if (!DynamicCostEffectClass) return true;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return false;

	bool bCanAfford = false;
    
	// 현재 스태미나 잔량을 직접 확인하여 지불 가능 여부를 판단
	if (ASC->GetAttributeSet(USFPrimarySet_Hero::StaticClass())) // AttributeSet 클래스 이름 확인 필요
	{
		// 현재 스태미나 값을 가져옴
		float CurrentStamina = ASC->GetNumericAttribute(USFPrimarySet_Hero::GetStaminaAttribute()); 
		
		if (CurrentStamina >= CostAmount)
		{
			bCanAfford = true;
		}
	}

	// 부족하면 즉시 실패 처리
	if (!bCanAfford) return false;

	// 비용 지불을 위한 GE Spec을 생성
	FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle CostSpecHandle = ASC->MakeOutgoingSpec(DynamicCostEffectClass, GetAbilityLevel(), EffectContextHandle);

	// Spec이 유효하면 비용을 설정하고 적용
	if (CostSpecHandle.IsValid())
	{
		CostSpecHandle.Data->SetSetByCallerMagnitude(CostDataTag, -CostAmount);
		ASC->ApplyGameplayEffectSpecToSelf(*CostSpecHandle.Data.Get());
	}

	return true;
}

// void USFGA_Hero_BasicAttack::ServerSetWarpRotation_Implementation(FName TargetName, FRotator TargetRotation)
// {
// 	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
// 	if (!Character) return;
//
// 	UMotionWarpingComponent* MotionWarpingComp = Character->GetComponentByClass<UMotionWarpingComponent>();
// 	if (!MotionWarpingComp) return;
//
// 	// 서버에서도 동일한 원거리 투영 로직을 적용하여 동기화 오차를 줄임
// 	const float DirectionalWarpDistance = 10000.f;
// 	FVector TargetLocation = Character->GetActorLocation() + (TargetRotation.Vector() * DirectionalWarpDistance);
//
// 	MotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(TargetName, TargetLocation, TargetRotation);
// }
//
// bool USFGA_Hero_BasicAttack::ServerSetWarpRotation_Validate(FName TargetName, FRotator TargetRotation)
// {
// 	return true;
// }

// ----------------------------------------------------------------------------------------------------------------
// Event Handlers
// ----------------------------------------------------------------------------------------------------------------

void USFGA_Hero_BasicAttack::OnInputPressed(float TimeWaited)
{
	// [입력 캐싱] 키를 누른 순간의 벡터를 저장함 (Input Ignore 방지)
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		FVector MoveInput = Character->GetLastMovementInputVector();
		if (!MoveInput.IsNearlyZero())
		{
			CachedTargetRotation = MoveInput.Rotation();
		}
	}
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(ComboWindowTag))
	{
		// 콤보 윈도우가 열려있다면 입력을 예약함
		bInputReserved = true;
	}
	else
	{
		// 콤보 윈도우가 열리지 않았다면 다음 입력을 다시 대기함
		UAbilityTask_WaitInputPress* RetryTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
		if (RetryTask)
		{
			RetryTask->OnPress.AddDynamic(this, &ThisClass::OnInputPressed);
			RetryTask->ReadyForActivation();
		}
	}
}

void USFGA_Hero_BasicAttack::OnComboWindowTagRemoved()
{
	// 입력이 예약되어 있다면 다음 콤보 단계를 실행함
	if (bInputReserved)
	{
		bInputReserved = false;
		CurrentStepIndex = (CurrentStepIndex + 1) % AttackSteps.Num();
		
		if (ActiveMontageTask) 
		{ 
			ActiveMontageTask->EndTask(); 
			ActiveMontageTask = nullptr; 
		}
		
		ExecuteAttackStep(CurrentStepIndex);
	}
}

void USFGA_Hero_BasicAttack::OnMontageFinished()
{
	bIsCharging = false;
	RemoveStepGameplayTags();
	RestoreSlidingMode(); 
	
	// 예약된 입력이 없다면 어빌리티를 종료함
	if (!bInputReserved)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void USFGA_Hero_BasicAttack::OnTrace(FGameplayEventData Payload)
{
	// 부모 클래스의 트레이스 로직을 수행함
	Super::OnTrace(Payload);
}

// ----------------------------------------------------------------------------------------------------------------
// Helper Functions
// ----------------------------------------------------------------------------------------------------------------

void USFGA_Hero_BasicAttack::ApplyStepGameplayTags(const FGameplayTagContainer& Tags)
{
	if (Tags.IsEmpty()) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo()) 
	{ 
		ASC->AddLooseGameplayTags(Tags); 
		AppliedStepTags = Tags; 
	}
}

void USFGA_Hero_BasicAttack::RemoveStepGameplayTags()
{
	if (AppliedStepTags.IsEmpty()) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo()) 
	{ 
		ASC->RemoveLooseGameplayTags(AppliedStepTags); 
		AppliedStepTags.Reset(); 
	}
}