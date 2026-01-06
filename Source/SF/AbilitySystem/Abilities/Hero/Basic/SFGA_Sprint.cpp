// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGA_Sprint.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Hero/SFPrimarySet_Hero.h"
#include "Character/SFCharacterBase.h"

USFGA_Sprint::USFGA_Sprint()
{
	// [설정] 어빌리티 인스턴싱 및 네트워크 정책
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// [태그] 어빌리티 태그 설정
	// 이 어빌리티가 활성화되면 'Ability.Sprint' 태그를 가집니다.
	FGameplayTag SprintTag = FGameplayTag::RequestGameplayTag(FName("Ability.Sprint"));
	SetAssetTags(FGameplayTagContainer(SprintTag));
}

void USFGA_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		// 현재 스태미나 값을 가져옵니다.
		float CurrentStamina = ASC->GetNumericAttribute(USFPrimarySet_Hero::GetStaminaAttribute());
		
		// 스태미나가 없으면 즉시 종료
		if (CurrentStamina <= 0.0f)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}

	// 1. [비용 검사] 실행 비용(Cost)과 쿨타임(Cooldown)을 확인하고 지불합니다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		// 비용 지불 실패 시 즉시 종료
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. [버프 적용] 이동 속도 증가 GE 적용
	if (SprintEffectClass)
	{
		// 자신에게 GE를 적용하고, 나중에 끄기 위해 핸들을 저장해둡니다.
		ActiveSprintEffectHandle = ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, SprintEffectClass.GetDefaultObject(), 1.0f);
	}
	
	// 3. [비용 적용] 스태미나 소모 GE 적용
	if (SprintCostEffectClass)
	{
		ActiveSprintCostEffectHandle = ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, SprintCostEffectClass.GetDefaultObject(), 1.0f);
	}
	
	// 4. [감지 시작] 스태미나 변화 모니터링 등록
	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		// 스태미나(Stamina) 값이 변할 때마다 OnStaminaChanged 함수가 호출되도록 연결
		// USFPrimarySet_Hero::GetStaminaAttribute()를 사용해 정확한 어트리뷰트를 지정
		StaminaChangeDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(USFPrimarySet_Hero::GetStaminaAttribute()).AddUObject(this, &USFGA_Sprint::OnStaminaChanged);
	}

	// 5. [입력 대기] 키를 뗄 때까지 대기하는 태스크 실행
	UAbilityTask_WaitInputRelease* Task = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	if (Task)
	{
		// 키를 떼면(OnRelease) -> OnInputReleased 함수 실행
		Task->OnRelease.AddDynamic(this, &USFGA_Sprint::OnInputReleased);
		Task->ReadyForActivation();
	}
}

void USFGA_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{

	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		// 1. [감지 해제] 스태미나 델리게이트 정리
		if (StaminaChangeDelegateHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(USFPrimarySet_Hero::GetStaminaAttribute()).Remove(StaminaChangeDelegateHandle);
		}

		// 2. [버프 제거] 저장해둔 핸들을 사용하여 해당 GE 제거
		if (ActiveSprintEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(ActiveSprintEffectHandle);
		}
		
		// 3. [비용 제거] 저장해둔 핸들을 사용하여 해당 GE 제거
		if (ActiveSprintCostEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(ActiveSprintCostEffectHandle);
		}
		
		StaminaChangeDelegateHandle.Reset();
		ActiveSprintEffectHandle.Invalidate();
		ActiveSprintCostEffectHandle.Invalidate();
	}
		
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGA_Sprint::OnInputReleased(float TimeHeld)
{
	// 키를 뗐으므로 정상적으로 어빌리티를 종료
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Sprint::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	// 이미 종료 중이거나 비활성이면 무시
	if (!IsActive())
	{
		return;
	}

	// 스태미나가 0 이하가 되면 어빌리티 취소
	if (Data.NewValue <= 0.0f)
	{
		// CancelAbility는 내부적으로 EndAbility를 호출합니다.
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
}