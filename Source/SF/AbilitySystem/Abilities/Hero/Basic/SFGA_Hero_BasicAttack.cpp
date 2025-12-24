#include "SFGA_Hero_BasicAttack.h"

// Engine & Ability System Includes
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// Project Includes
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/Hero/Component/SFHeroMovementComponent.h"
#include "Input/SFInputGameplayTags.h"
#include "SFBasicAttackData.h"
#include "Equipment/SFEquipmentTags.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "Weapons/Actor/SFEquipmentBase.h"
#include "Character/SFCharacterBase.h"

USFGA_Hero_BasicAttack::USFGA_Hero_BasicAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	
	// 기본값 설정
	ComboWindowTag = SFGameplayTags::Character_State_ComboWindow;
	
	DamageDataTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage.BaseDamage"));
}

void USFGA_Hero_BasicAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 1. 데이터 에셋 로드 및 검증
	if (AttackDataAsset)
	{
		AttackSteps = AttackDataAsset->AttackSteps;
	}

	if (AttackSteps.Num() == 0)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 2. 이벤트 리스너 등록
	
	// 입력 감지 (InputTag -> GameplayEvent)
	UAbilityTask_WaitGameplayEvent* InputTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SFGameplayTags::InputTag_Attack);
	if (InputTask)
	{
		InputTask->EventReceived.AddDynamic(this, &ThisClass::OnInputPressedEvent);
		InputTask->ReadyForActivation();
	}

	// 히트 판정 감지 (WeaponActor -> GameplayEvent)
	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SFGameplayTags::GameplayEvent_TraceHit);
	if (HitTask)
	{
		HitTask->EventReceived.AddDynamic(this, &ThisClass::OnTraceHitReceived);
		HitTask->ReadyForActivation();
	}

	// 3. 첫 번째 공격 실행
	ExecuteAttackStep(CurrentStepIndex);
}

void USFGA_Hero_BasicAttack::ExecuteAttackStep(int32 StepIndex)
{
	if (!AttackSteps.IsValidIndex(StepIndex))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	
	// 스태미나 소모
	if (!CommitAbilityCost(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		// 스태미나 부족 시 어빌리티 종료
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 상태 초기화 및 설정
	RemoveStepGameplayTags();
	UpdateWarpTargetFromInput();

	const FSFBasicAttackStep& CurrentStep = AttackSteps[StepIndex];
	ApplyStepGameplayTags(CurrentStep.TempAbilityTags);

	// 매 단계마다 태그 제거(콤보 윈도우 종료) 감지 태스크를 갱신
	UAbilityTask_WaitGameplayTagRemoved* TagRemovedTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(this, ComboWindowTag);
	if (TagRemovedTask)
	{
		TagRemovedTask->Removed.AddDynamic(this, &ThisClass::OnComboWindowTagRemoved);
		TagRemovedTask->ReadyForActivation();
	}

	// 차징 로직 (필요 시)
	if (CurrentStep.bIsChargeStep)
	{
		bIsCharging = true;
		ChargeStartTime = GetWorld()->GetTimeSeconds();
	}
	
	// 몽타주 재생 (BlendInTime을 0.1f로 설정하여 부드러운 전이 유도)
	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		CurrentStep.Montage,
		1.0f,
		NAME_None,
		true,
		1.0f,
		0.1f // BlendInTime
	);
	
	if (ActiveMontageTask)
	{
		ActiveMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
		ActiveMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
		ActiveMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
		ActiveMontageTask->ReadyForActivation();
	}
}

void USFGA_Hero_BasicAttack::OnInputPressedEvent(FGameplayEventData Payload)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	// 콤보 윈도우 구간(태그 보유 중)에 입력이 들어왔을 때만 예약
	if (ASC->HasMatchingGameplayTag(ComboWindowTag))
	{
		bInputReserved = true;
	}
}

void USFGA_Hero_BasicAttack::OnComboWindowTagRemoved()
{
	// [핵심 로직] 노티파이 종료 시점에 예약 여부를 확인하고 즉시 다음 단계 실행
	if (bInputReserved)
	{
		bInputReserved = false;
		CurrentStepIndex = (CurrentStepIndex + 1) % AttackSteps.Num();
		
		// 이전 태스크를 명시적으로 정리하여 콜백 충돌 방지
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

	// 예약된 입력이 없는 상태에서 몽타주가 자연 종료되면 어빌리티 종료
	// (bInputReserved가 true라면 OnComboWindowTagRemoved에서 이미 처리됨)
	if (!bInputReserved)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void USFGA_Hero_BasicAttack::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (bIsCharging)
	{
		float ChargeDuration = GetWorld()->GetTimeSeconds() - ChargeStartTime;
		bIsCharging = false;

		if (AttackSteps.IsValidIndex(CurrentStepIndex))
		{
			const FSFBasicAttackStep& CurrentStep = AttackSteps[CurrentStepIndex];
			
			// 차징 성공 시 다음 단계(발사 등)로 진행
			if (ChargeDuration >= CurrentStep.MinChargeTime)
			{
				CurrentStepIndex++; 
				ExecuteAttackStep(CurrentStepIndex);
			}
			else
			{
				EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			}
		}
	}
}

void USFGA_Hero_BasicAttack::OnTraceHitReceived(FGameplayEventData Payload)
{
	// 1. 타겟 유효성 검사
	const AActor* RawTarget = Payload.Target.Get();
	if (!Payload.Target) return;
	
	if (ASFCharacterBase* SourceCharacter = Cast<ASFCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		if (const ASFCharacterBase* TargetCharacter = Cast<ASFCharacterBase>(RawTarget))
		{
			return;
		}
	}
	else if (const AActor* TargetOwner = RawTarget->GetOwner())
	{
		if (const ASFCharacterBase* OwnerCharacter = Cast<ASFCharacterBase>(TargetOwner))
		{
			if (SourceCharacter->GetTeamAttitudeTowards(*OwnerCharacter) != ETeamAttitude::Hostile)
			{
				return;
			}
		}
	}

	// 2. 무기 기본 대미지 가져오기
	float WeaponBaseDamage = 0.0f;
	
	// Payload.Instigator가 무기 액터라면 바로 가져옴 (SFMeleeWeaponActor 등에서 설정해서 보냄)
	if (const ASFEquipmentBase* Weapon = Cast<ASFEquipmentBase>(Payload.Instigator))
	{
		WeaponBaseDamage = Weapon->WeaponBaseDamage;
	}
	else if (ASFEquipmentBase* EquippedWeapon = GetMainHandWeapon()) // Fallback
	{
		WeaponBaseDamage = EquippedWeapon->WeaponBaseDamage;
	}

	// 3. 현재 콤보 단계의 배율 적용
	float DamageMultiplier = 1.0f;
	if (AttackSteps.IsValidIndex(CurrentStepIndex))
	{
		DamageMultiplier = AttackSteps[CurrentStepIndex].DamageMultiplier;
	}

	const float FinalBaseDamage = WeaponBaseDamage * DamageMultiplier;

	// 4. GE Spec 생성 및 적용
	if (DamageEffectClass)
	{
		UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(RawTarget));
		if (SourceASC && TargetASC)
		{
			// Context에 HitResult 포함 (SFDamageEffectExecCalculation 및 HitReaction 처리에 필수)
			FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
			if (Payload.TargetData.IsValid(0))
			{
				ContextHandle.AddHitResult(*Payload.TargetData.Get(0)->GetHitResult());
			}
			// 무기를 SourceObject로 추가
			ContextHandle.AddSourceObject(Payload.Instigator); 
			
			FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), ContextHandle);
			if (SpecHandle.IsValid())
			{
				// SetByCaller로 BaseDamage 전달
				// SFDamageEffectExecCalculation::CalculateBaseDamage에서 이 태그의 값을 읽어 기본 대미지로 사용함
				SpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageDataTag, FinalBaseDamage);
				
				// 타겟에게 적용
				SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			}
		}
	}
}

ASFEquipmentBase* USFGA_Hero_BasicAttack::GetMainHandWeapon() const
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (AActor* Avatar = ASC->GetAvatarActor())
		{
			// 컴포넌트 클래스 이름으로 찾
			if (USFEquipmentComponent* EquipmentComp = Avatar->FindComponentByClass<USFEquipmentComponent>())
			{
				// 반환된 Actor*를 ASFEquipmentBase*로 캐스팅
				return Cast<ASFEquipmentBase>(EquipmentComp->GetFirstEquippedActorBySlot(FGameplayTag::RequestGameplayTag(FName("Equipment.Slot.MainHand"))));
			}
		}
	}
	return nullptr;
}

void USFGA_Hero_BasicAttack::UpdateWarpTargetFromInput()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character) return;
	
	// SFHeroMovementComponent 캐스팅
	USFHeroMovementComponent* SFCMC = Cast<USFHeroMovementComponent>(Character->GetCharacterMovement());
	if (!SFCMC) return;
	
	const FVector InputVector = Character->GetLastMovementInputVector();
	
	FVector TargetLocation;
	FRotator TargetRotation;
	
	if (!InputVector.IsNearlyZero())
	{
		// 입력이 있다면 입력 방향을 타겟으로 설정
		TargetRotation = InputVector.Rotation();
		TargetRotation.Pitch = 0.f;
		TargetRotation.Roll = 0.f;
		
		TargetLocation = Character->GetActorLocation() + (Character->GetActorForwardVector() * WarpDistance);
	}
	else
	{
		// 입력값이 없다면 현재 캐릭터 전방을 유지
		TargetLocation = Character->GetActorLocation() + (Character->GetActorForwardVector() * WarpDistance);
		TargetRotation = Character->GetActorRotation();
	}
	
	SFCMC->SetWarpTarget(TargetLocation, TargetRotation);
}

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

void USFGA_Hero_BasicAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	RemoveStepGameplayTags();
	
	// 워핑 타겟 정리
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (USFHeroMovementComponent* SFCMC = Cast<USFHeroMovementComponent>(Character->GetCharacterMovement()))
		{
			SFCMC->ClearWarpTarget();
		}
	}
	
	// 상태 변수 초기화
	CurrentStepIndex = 0;
	bInputReserved = false;
	bIsCharging = false;
	ActiveMontageTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}