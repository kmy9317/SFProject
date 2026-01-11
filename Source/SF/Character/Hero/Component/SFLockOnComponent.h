#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "GameplayTagContainer.h"
#include "Interface/SFLockOnInterface.h"
#include "SFLockOnComponent.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

/**
 * USFLockOnComponent
 * 소울라이크 스타일의 락온(Lock-On) 시스템을 담당하는 컴포넌트
 * * [주요 기능]
 * 1. 화면 중앙 가중치(Dot Product) 기반 타겟 탐색
 * 2. 타겟 스위칭(Target Switching): 입력 방향으로 타겟 변경
 * 3. 시야 가림 유예(Grace Period): 장애물에 가려져도 잠시 락온 유지
 * 4. 캐릭터 이동 제어: 락온 시 Strafing(게걸음) 모드로 전환
 * 5. 락온 타겟 VFX: 나이아가라를 활용한 타겟 확인
 */
UCLASS(Blueprintable, Meta = (BlueprintSpawnableComponent))
class SF_API USFLockOnComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	USFLockOnComponent(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** [Client -> Server] 락온 시도 (Toggle) */
	UFUNCTION(BlueprintCallable, Category = "SF|LockOn")
	void TryLockOn();

	/** [Client -> Server] 락온 해제 */
	UFUNCTION(BlueprintCallable, Category = "SF|LockOn")
	void EndLockOn();

	/** [Client] 카메라를 캐릭터 정면으로 리셋 (락온 실패/해제 시) */
	UFUNCTION(BlueprintCallable, Category = "SF|LockOn")
	void ResetCamera();
	
	UFUNCTION(BlueprintPure, Category = "SF|LockOn")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

	UFUNCTION(BlueprintPure, Category = "SF|LockOn")
	bool IsLockedOn() const { return CurrentTarget != nullptr; }

protected:
	// ==========================================
	//  업데이트 로직 (Tick 분리)
	// ==========================================

	/** [Server] 타겟 유효성 검증 (거리, 시야) */
	void ServerUpdate_TargetValidation(float DeltaTime);

	/** [Client] 카메라 및 캐릭터 회전 처리 */
	void ClientUpdate_Rotation(float DeltaTime);

	/** [Client] 타겟 스위칭 입력 감지 */
	void ClientUpdate_SwitchingInput(float DeltaTime);
	
	/** 회전 모드 변경 및 GAS 태그 클라이언트 동기화 */
	void UpdateLockOnState(bool bIsLockedOn);
	
	/** 회전 설정 변경 */
	void UpdateCharacterRotationMode(bool bLockOnEnabled);

protected:
	// ==========================================
	//  내부 로직 & GAS 이벤트
	// ==========================================

	// 최적의 타겟 찾기
	AActor* FindBestTarget();
	
	// 타겟 유효성 검사
	bool IsTargetValid(AActor* TargetActor) const;
	bool IsHostile(AActor* TargetActor) const;
	
	// 소켓 위치 가져오기 헬퍼
	FVector GetActorSocketLocation(AActor* Actor, FName SocketName) const;

	// [Event] 타겟의 사망 태그 이벤트를 바인딩
	void RegisterTargetEvents(AActor* NewTarget);
	void UnregisterTargetEvents(AActor* OldTarget);

	// [Callback] 타겟이 죽었을 때 호출됨
	void OnTargetDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	// VFX 관리
	void CreateLockOnEffect();
	void DestroyLockOnEffect();
	void UpdateLockOnEffectLocation();

	// ==========================================
	//  RPC (네트워크)
	// ==========================================

	UFUNCTION(Server, Reliable)
	void Server_TryLockOn(AActor* TargetActor);

	UFUNCTION(Server, Reliable)
	void Server_EndLockOn();

	UFUNCTION(Server, Reliable)
	void Server_SwitchTarget(const FVector2D& Input);

	// 내부적으로 타겟을 변경할 때 사용하는 함수 (모든 변경은 이 함수를 통해야 함)
	void Server_SetCurrentTarget(AActor* NewTarget, FName SocketName);

	// ==========================================
	//  Replication Callbacks
	// ==========================================
	
	UFUNCTION()
	void OnRep_CurrentTarget(AActor* OldTarget);

	UFUNCTION()
	void OnRep_CurrentTargetSocketName();

protected:
	// ==========================================
	//  데이터
	// ==========================================

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentTarget, Category = "SF|LockOn")
	TObjectPtr<AActor> CurrentTarget;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentTargetSocketName, Category = "SF|LockOn")
	FName CurrentTargetSocketName;

	// 타겟 사망 이벤트 핸들 (GAS)
	FDelegateHandle TargetDeathDelegateHandle;

	// ------------------------------------------
	// 설정 값 (Config)
	// ------------------------------------------
	
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn")
	float LockOnDistance = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn")
	float LockOnBreakDistance = 5000.0f; 

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn")
	float ScreenCenterWeight = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn")
	FGameplayTagContainer TargetTags;

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn")
	FGameplayTag SprintTag;

	// 점수 가중치
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Selection")
	float Weight_Distance = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Selection")
	float Weight_Angle = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Selection")
	float Weight_BossBonus = 2.0f;

	// 시야 가림(Occlusion) 설정
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn")
	float LostTargetMemoryTime = 5.0f;

	float TimeSinceTargetHidden = 0.0f;

	// 카메라 제어 변수
	FRotator LastLockOnRotation;
	bool bIsSwitchingTarget = false;
	
	// 카메라 리셋용 보간 변수
	bool bIsResettingCamera = false;
	FRotator CameraResetTargetRotation;
	
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Camera")
	float CameraInterpSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Camera")
	float CameraResetInterpSpeed = 5.0f;
	
	UPROPERTY(EditAnywhere, Category = "SF|LockOn|Switching")
	float TargetSwitchInterpSpeed = 15.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Camera")
	float PitchLimitMin = -60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Camera")
	float PitchLimitMax = 45.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Camera")
	float CloseRangePitchLimitMax = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Camera")
	float CloseRangeThreshold = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Camera")
	float AutoBreakPitchAngle = 80.0f;

	// 스위칭 설정
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Switching")
	float SwitchInputThreshold = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Switching")
	float SwitchAngularLimit = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|Switching")
	float SwitchCooldown = 0.25f;

	float CurrentSwitchCooldown = 0.0f;
	double LastLockOnToggleTime = 0.0;

	// ------------------------------------------
	// VFX
	// ------------------------------------------
	
	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|VFX")
	FName DefaultLockOnSocketName = FName("spine_02");

	UPROPERTY(EditDefaultsOnly, Category = "SF|LockOn|VFX")
	TObjectPtr<UNiagaraSystem> LockOnEffectTemplate;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> LockOnEffectComponent;
};