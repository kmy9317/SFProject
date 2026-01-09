#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "Interface/SFLockOnInterface.h"
#include "SFLockOnComponent.generated.h"

/**
 * USFLockOnComponent
 * 소울라이크 스타일의 락온(Lock-On) 시스템을 담당하는 컴포넌트
 * * [주요 기능]
 * 1. 화면 중앙 가중치(Dot Product) 기반 타겟 탐색
 * 2. 하드 락온(Hard Lock): 마우스 회전 입력을 무시하고 타겟 고정
 * 3. 타겟 스위칭(Target Switching): 입력 방향으로 타겟 변경
 * 4. 시야 가림 유예(Grace Period): 장애물에 가려져도 잠시 락온 유지
 * 5. 캐릭터 이동 제어: 락온 시 Strafing(게걸음) 모드로 전환
 */
UCLASS(Blueprintable, Meta = (BlueprintSpawnableComponent))
class SF_API USFLockOnComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	USFLockOnComponent(const FObjectInitializer& ObjectInitializer);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * 락온을 시도하거나 해제합니다. (Toggle)
	 */
	UFUNCTION(BlueprintCallable, Category = "SF|LockOn")
	bool TryLockOn();

	// 락온 강제 해제
	UFUNCTION(BlueprintCallable, Category = "SF|LockOn")
	void EndLockOn();

	// 현재 타겟 반환
	UFUNCTION(BlueprintPure, Category = "SF|LockOn")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

protected:
	// ==========================================
	//  틱(Tick) 내부 로직 분리
	// ==========================================
	
	// 1. 타겟 유효성 검사 (거리, 시야 가림 유예 처리)
	void UpdateLogic_TargetValidation(float DeltaTime);

	// 2. 타겟 스위칭 입력 처리 (마우스/스틱)
	void HandleTargetSwitching(float DeltaTime);

	// 3. 카메라 회전 제어 (Hard Lock 및 스위칭 보간)
	void UpdateLogic_CameraRotation(float DeltaTime);

	// 4. 캐릭터 회전 제어 (카메라 방향과 동기화)
	void UpdateLogic_CharacterRotation(float DeltaTime);

	// 5. 위젯 위치 업데이트
	void UpdateLogic_WidgetPosition(float DeltaTime);

protected:
	// ==========================================
	//  내부 유틸리티 함수 (알고리즘 개선)
	// ==========================================

	// 최적의 타겟 탐색 (Score 기반)
	AActor* FindBestTarget();
	
	// 타겟 유효성 검사 (Interface 활용)
	bool IsTargetValid(AActor* TargetActor) const;

	// 적대 관계 확인 (Team ID 활용)
	bool IsHostile(AActor* TargetActor) const;

	// UI 위젯 생성 및 파괴
	void CreateLockOnWidget();
	void DestroyLockOnWidget();

protected:
	// ==========================================
	//  변수 및 설정
	// ==========================================

	// 현재 락온된 대상
	UPROPERTY(BlueprintReadOnly, Category = "SF|LockOn")
	TObjectPtr<AActor> CurrentTarget;

	// 현재 타겟의 조준 소켓 이름 (예: Head, Spine_02)
	UPROPERTY(BlueprintReadOnly, Category = "SF|LockOn")
	FName CurrentTargetSocketName;

	// 락온 탐색 거리
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn")
	float LockOnDistance = 1500.0f;

	// 락온 해제 거리
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn")
	float LockOnBreakDistance = 1700.0f;

	// 화면 중앙 가중치 (0~1, 클수록 중앙에 있는 적 우선)
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn")
	float ScreenCenterWeight = 0.6f;

	// 타겟 필터 태그 (Enemy 등)
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn")
	FGameplayTagContainer TargetTags;
	
	// 달리기 상태 태그
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn")
	FGameplayTag SprintTag;

	// [New] 타겟 선정 가중치 (Score Weights)
	
	// 거리 점수 가중치 (1.0 = 기본)
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Selection")
	float Weight_Distance = 1.0f;

	// 각도 점수 가중치 (1.5 = 화면 중앙 우선시)
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Selection")
	float Weight_Angle = 1.5f;

	// 보스 우선 가중치 (2.0 = 보스 선호)
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Selection")
	float Weight_BossBonus = 2.0f;

	// ------------------------------------------
	// 시야 가림(Occlusion) 유예 설정
	// ------------------------------------------
	
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn")
	float LostTargetMemoryTime = 1.0f;

	float TimeSinceTargetHidden = 0.0f;

	// ------------------------------------------
	// 카메라 및 스위칭 설정
	// ------------------------------------------

	FRotator LastLockOnRotation;
	bool bIsSwitchingTarget = false;

	UPROPERTY(EditAnywhere, Category = "SF|LockOn|Switching")
	float TargetSwitchInterpSpeed = 15.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Switching")
	float SwitchInputThreshold = 0.5f;
	
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Switching")
	float SwitchAngularLimit = 0.5f;
	
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Switching")
	float SwitchCooldown = 0.3f;

	float CurrentSwitchCooldown = 0.0f;
	
	double LastLockOnToggleTime = 0.0;

	// ------------------------------------------
	// UI 설정
	// ------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|UI")
	TSubclassOf<UUserWidget> LockOnWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> LockOnWidgetInstance;

	// 기본 소켓 이름 (Interface 미사용 시 Fallback)
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|UI")
	FName LockOnSocketName = FName("spine_02");
};