// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/SFGA_Hero_Base.h"
#include "SFGA_Sprint.generated.h"

/**
 * 전력질주(Sprint) 어빌리티
 * * 기능:
 * 1. Shift 키를 누르는 동안 이동 속도 버프(GE)를 적용합니다.
 * 2. 스태미나가 0이 되면 자동으로 달리기가 취소됩니다.
 * 3. 키를 떼거나 다른 이유로 취소되면 버프를 제거합니다.
 */
UCLASS()
class SF_API USFGA_Sprint : public USFGA_Hero_Base
{
	GENERATED_BODY()

public:
	USFGA_Sprint();

protected:
	// --- [Life Cycle] 어빌리티 수명 주기 함수 ---

	/** 어빌리티가 승인되고 활성화될 때 호출됩니다. (초기화 및 로직 시작) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 어빌리티가 종료될 때 호출됩니다. (정리 작업) */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// --- [Callbacks] 이벤트 콜백 함수 ---

	/** 입력 키(Shift)를 뗐을 때 호출됩니다. */
	UFUNCTION()
	void OnInputReleased(float TimeHeld);

	/** 스태미나 어트리뷰트 값이 변할 때마다 호출됩니다. (스태미나 0 감지용) */
	void OnStaminaChanged(const FOnAttributeChangeData& Data);

protected:
	// --- [Data] 설정 및 상태 변수 ---

	// 1. 이동 속도 버프 GE
	UPROPERTY(EditDefaultsOnly, Category = "Sprint")
	TSubclassOf<UGameplayEffect> SprintEffectClass;

	// 2. 스태미나 소모 비용 GE
	UPROPERTY(EditDefaultsOnly, Category = "Sprint")
	TSubclassOf<UGameplayEffect> SprintCostEffectClass;

private:
	/** 적용된 속도 버프를 추적하기 위한 핸들 (종료 시 제거용) */
	FActiveGameplayEffectHandle ActiveSprintEffectHandle;
	
	/** 비용 GE 핸들 (종료 시 제거용) */
	FActiveGameplayEffectHandle ActiveSprintCostEffectHandle;

	/** 스태미나 변화 감지 델리게이트 핸들 (종료 시 바인딩 해제용) */
	FDelegateHandle StaminaChangeDelegateHandle;
};