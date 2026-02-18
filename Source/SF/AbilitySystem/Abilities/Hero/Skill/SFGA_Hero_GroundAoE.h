// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/SFGA_Equipment_Base.h"
#include "ScalableFloat.h"
#include "SFGA_Hero_GroundAoE.generated.h"

class USFAbilityTask_WaitCancelInput;
class ASFGroundAOE;
class UAbilityTask_WaitGameplayEvent;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitInputPress;
class UAbilityTask_WaitInputRelease;

/**
 * 지면 지정형(AOE) 스킬
 * 1단계: 루프 애니메이션 + 마우스로 위치 지정 (Reticle)
 * 2단계: 스킬 재사용 시 확정 및 발사
 */
UCLASS()
class SF_API USFGA_Hero_GroundAoE : public USFGA_Equipment_Base
{
	GENERATED_BODY()
	
public:
	USFGA_Hero_GroundAoE(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,bool bReplicateEndAbility,bool bWasCancelled) override;

protected:

	// 틱마다 실행 (WaitTick 대용, Reticle 위치 업데이트)
	void TickReticle();

	// 키를 뗐을 때 호출 (준비 완료)
	UFUNCTION()
	void OnKeyReleased(float TimeWaited);

	//  키를 다시 눌렀을 때 호출 (발사 확정)
	UFUNCTION()
	void OnConfirmInputPressed(float TimeWaited);

	UFUNCTION()
	void OnCancelInputReceived();

	// 공격 몽타주 종료 시
	UFUNCTION()
	void OnAttackMontageCompleted();

	// 노티파이 수신 시 장판 소환
	UFUNCTION()
	virtual void OnSpawnEventReceived(FGameplayEventData Payload);

	// 유틸: 마우스 위치 가져오기
	bool GetGroundLocationUnderCursor(FVector& OutLocation);

protected:
	// === 1. 설정 변수들 ===
	
	// 소환할 장판 액터 클래스
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE")
	TSubclassOf<ASFGroundAOE> AOEActorClass;

	// 범위 지정 시 보여줄 Reticle(인디케이터) 액터 클래스
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE")
	TSubclassOf<AActor> ReticleClass;

	// 데미지 (CurveTable 지원)
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Stat")
	FScalableFloat BaseDamage = 20.f;

	// 장판 반지름
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Stat")
	float AOERadius = 300.f;

	// 장판 지속 시간
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Stat")
	float Duration = 5.f;

	// 데미지 주기 (초)
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Stat")
	float TickInterval = 0.5f;
	
	// 준비 단계 루프 몽타주
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Animation")
	TObjectPtr<UAnimMontage> AimingLoopMontage;

	// 확정 후 공격 몽타주
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	// 공격 몽타주에서 장판을 소환할 타이밍 노티파이 태그
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Animation")
	FGameplayTag SpawnEventTag;

protected:
	// 확정된 목표 위치
	FVector TargetLocation; 

private:
	// 상태 관리
	UPROPERTY()
	TObjectPtr<AActor> SpawnedReticle;
	
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputRelease> InputReleaseTask; // [추가]

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputPress> InputPressTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> AimingMontageTask;
	
	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitEventTask;

	UPROPERTY()
	TObjectPtr<USFAbilityTask_WaitCancelInput> CancelInputTask;
};
