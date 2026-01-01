// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/SFGA_Equipment_Base.h"
#include "SFGA_Skill_Melee.generated.h"

class USFAbilityTask_UpdateWarpTarget;
class ASFEquipmentBase;

/**
 * 근접 스킬 베이스 클래스
 * Windup 중 방향 업데이트
 * 히트 판정 및 데미지 처리
 */
UCLASS()
class SF_API USFGA_Skill_Melee : public USFGA_Equipment_Base
{
	GENERATED_BODY()

public:
	USFGA_Skill_Melee(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// ========== Hit Processing ==========
	
	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	void ParseTargetData(const FGameplayAbilityTargetDataHandle& InTargetDataHandle, TArray<int32>& OutActorsHitIndexes);
	
	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	void ProcessHitResult(FHitResult HitResult, float Damage, ASFEquipmentBase* WeaponActor);

	UFUNCTION(BlueprintCallable, Category = "SF|Ability") 
	void ResetHitActors();

	void DrawDebugHitPoint(const FHitResult& HitResult);

	UFUNCTION()
	virtual void OnTrace(FGameplayEventData Payload);

	// ========== Windup Warp System ==========
	
	// Windup Warp Task 시작 (ActivateAbility에서 호출)
	UFUNCTION(BlueprintCallable, Category = "SF|Ability|Warp")
	void StartWindupWarpTask();

	// Warp Task 정리 (EndAbility에서 호출)
	UFUNCTION(BlueprintCallable, Category = "SF|Ability|Warp")
	void CleanupWindupWarpTask();
	
	// Warp Target Commit 콜백
	UFUNCTION()
	virtual void OnWarpTargetCommitted(FVector InCommittedDirection, FVector InCommittedLocation);

	// 현재 확정된 방향/위치 Getter
	UFUNCTION(BlueprintPure, Category = "SF|Ability|Warp")
	FVector GetCommittedDirection() const { return CommittedDirection; }

	UFUNCTION(BlueprintPure, Category = "SF|Ability|Warp")
	FVector GetCommittedLocation() const { return CommittedLocation; }

	// ========== Windup Warp Network Sync ==========

	// 서버에서 TargetData 수신 
	void OnServerWarpDirectionReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag);

	// 서버에서 방향 검증 
	bool ValidateWarpDirection(const FVector& ClientLocation, const FRotator& ClientRotation, int32 WindupIndex) const;

	// Motion Warping 적용 
	void ApplyWarpTarget(const FVector& Location, const FRotator& Rotation);

	UFUNCTION(BlueprintCallable, Category = "SF|Damage")
	float GetScaledBaseDamage() const;
	
protected:

	// ========== Damage Settings ==========
		
	UPROPERTY(EditDefaultsOnly, Category="SF|Damage")
	FScalableFloat BaseDamage = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Debug")
	bool bShowDebug = false;
	
	UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> CachedHitActors;

	float CurrentDamageMultiplier = 1.0f;

	// ========== Windup Warp Settings ==========
	
	// Windup Warp 기능 활성화 여부
	// true: Windup 구간 동안 입력 방향에 따라 공격 방향 실시간 조정
	// false: 기존 방식 (활성화 시점 방향 고정)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Warp")
	bool bUseWindupWarp = false;

	// WarpSettings를 EquipmentDefinition에서 가져올지, 직접 설정할지
	// true: 무기 DataAsset의 WarpSettings 사용
	// false: 아래 Override 값 사용
	UPROPERTY(EditDefaultsOnly, Category = "SF|Warp", meta = (EditCondition = "bUseWindupWarp"))
	bool bUseEquipmentWarpSettings = true;

	// bUseEquipmentWarpSettings가 false일 때 사용할 Override 설정
	UPROPERTY(EditDefaultsOnly, Category = "SF|Warp", meta = (EditCondition = "bUseWindupWarp && !bUseEquipmentWarpSettings"))
	FName WarpTargetNameOverride = TEXT("AttackTarget");

	UPROPERTY(EditDefaultsOnly, Category = "SF|Warp", meta = (EditCondition = "bUseWindupWarp && !bUseEquipmentWarpSettings"))
	float WarpRangeOverride = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Warp", meta = (EditCondition = "bUseWindupWarp && !bUseEquipmentWarpSettings"))
	float RotationInterpSpeedOverride = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Warp", meta = (EditCondition = "bUseWindupWarp && !bUseEquipmentWarpSettings"))
	float MaxWindupTurnAngleOverride = 90.f;

	UPROPERTY()
	TObjectPtr<USFAbilityTask_UpdateWarpTarget> WarpTargetTask;

private:
	
	// 확정된 방향/위치 (네트워크 동기화용)
	FVector CommittedDirection = FVector::ForwardVector;
	FVector CommittedLocation = FVector::ZeroVector;

	// Warp에 사용된 타겟 이름 
	FName ActiveWarpTargetName = NAME_None;

	// ============ Network Sync 변수 ============
	
	// 현재 Warp Range (검증용)
	float ActiveWarpRange = 0.f;

	// 현재 Windup 인덱스 (검증용)
	int32 CurrentWindupIndex = 0;

	// 서버 TargetData 델리게이트 핸들
	FDelegateHandle ServerWarpDataDelegateHandle;
};
