#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Skill_Melee.h"
#include "SFBasicAttackTypes.h"
#include "SFGA_Hero_BasicAttack.generated.h"

class USFBasicAttackData;
class UAbilityTask_PlayMontageAndWait;

/**
 * 영웅 기본 공격 어빌리티 클래스
 * 콤보 시스템, 입력 캐싱, 모션 워핑(Far Projection)을 포함함
 */
UCLASS(Abstract)
class SF_API USFGA_Hero_BasicAttack : public USFGA_Skill_Melee 
{
	GENERATED_BODY()

public:
	USFGA_Hero_BasicAttack();

	//~ Begin UGameplayAbility Interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End UGameplayAbility Interface

protected:
	//~ Begin SFGA_Skill_Melee Interface
	virtual void OnTrace(FGameplayEventData Payload) override;
	//~ End SFGA_Skill_Melee Interface

protected: //  Core Execution
	
	/** 특정 콤보 단계(Step)를 실행함 */
	void ExecuteAttackStep(int32 StepIndex);

	/** * 입력 및 캐싱된 방향을 기반으로 모션 워핑 타겟을 갱신함
	 * 캐릭터 회전(Spin) 방지를 위해 원거리 투영(Far Projection) 기법을 사용함
	 */
	void UpdateWarpTargetFromInput();

	/** 서버에 워핑 타겟 회전값을 동기화함 (Reliable RPC) */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetWarpRotation(FName TargetName, FRotator TargetRotation);

	/** 현재 단계의 태그를 적용함 */
	void ApplyStepGameplayTags(const FGameplayTagContainer& Tags);

	/** 적용된 단계 태그를 제거함 */
	void RemoveStepGameplayTags();

protected: // Event Handlers

	/** 입력 키가 눌렸을 때 호출됨 (입력 캐싱 처리) */
	UFUNCTION()
	void OnInputPressed(float TimeWaited);

	/** 콤보 윈도우 태그가 제거되었을 때(콤보 입력 가능 시간 종료) 호출됨 */
	UFUNCTION()
	void OnComboWindowTagRemoved();

	/** 몽타주 재생이 종료되었을 때 호출됨 */
	UFUNCTION()
	void OnMontageFinished();
	
protected: // Configuration

	/** 공격 데이터 에셋 (데미지, 몽타주 등 정의) */
	UPROPERTY(EditDefaultsOnly, Category = "SF|Attack")
	TObjectPtr<USFBasicAttackData> AttackDataAsset;

	/** 파싱된 공격 단계 배열 */
	UPROPERTY(BlueprintReadOnly, Category = "SF|Attack")
	TArray<FSFBasicAttackStep> AttackSteps;

	/** 콤보 입력 허용 구간 태그 */
	UPROPERTY(EditDefaultsOnly, Category = "SF|Attack")
	FGameplayTag ComboWindowTag;
	
private: // Runtime State

	/** 현재 진행 중인 콤보 인덱스 */
	int32 CurrentStepIndex = 0;

	/** 다음 콤보 입력이 예약되었는지 여부 */
	bool bInputReserved = false;

	/** 차지(Charge) 중인지 여부 */
	bool bIsCharging = false;

	/** 차징 시작 시간 */
	float ChargeStartTime = 0.0f;

	/** 선입력된 방향 회전값 (입력 무시 방지용 캐시) */
	FRotator CachedTargetRotation;

	/** 현재 적용된 단계별 태그 컨테이너 */
	FGameplayTagContainer AppliedStepTags;

	/** 활성화된 몽타주 태스크 포인터 */
	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;
};