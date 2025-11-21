// SFCombatSlotManager.h
// SF 프레임워크용 중앙 집중식 공격/가드 슬롯 관리자

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SFCombatSlotManager.generated.h"

class ASFEnemyController;

/**
 * 슬롯 정보 구조체
 * - 각 타겟별로 슬롯을 보유한 SFEnemyController 목록 관리
 */
USTRUCT()
struct FSFCombatSlotInfo
{
	GENERATED_BODY()

	/** 이 타겟에 대한 슬롯을 보유한 AI 컨트롤러 목록 */
	UPROPERTY()
	TArray<TObjectPtr<ASFEnemyController>> SlotHolders;

	/** 최대 슬롯 수 (기본값 3) */
	int32 MaxSlots = 3;
};

/**
 * USFCombatSlotManager
 * (구 CombatSlotManager)
 *
 * - SFEnemyController와 연동되는 중앙 슬롯 관리 시스템
 * - 공격(Guard) 권한을 순차적으로 배분하여 동시 공격 인원을 제한함
 */
UCLASS()
class SF_API USFCombatSlotManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ========================================
	// 설정 파라미터
	// ========================================

	/** 플레이어 간 거리 임계값 - 이보다 가까우면 슬롯 공유 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SF|Combat")
	float PlayerGroupDistance = 1500.0f;

public:
	/**
	 * 슬롯 요청 (Guard Slot)
	 * @param Requester 요청하는 SF AI 컨트롤러
	 * @param Target 타겟 액터
	 */
	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	bool RequestSlot(ASFEnemyController* Requester, AActor* Target);

	/**
	 * 슬롯 해제
	 * @param Releaser 해제하는 SF AI 컨트롤러
	 * @param Target 특정 타겟 (nullptr이면 모든 타겟에서 해제)
	 */
	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	void ReleaseSlot(ASFEnemyController* Releaser, AActor* Target = nullptr);

	/**
	 * 타겟별 슬롯 현황 조회
	 */
	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	bool GetSlotInfo(AActor* Target, int32& OutCurrentSlots, int32& OutMaxSlots) const;

	/**
	 * AI가 슬롯을 보유 중인지 확인
	 */
	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	bool HasSlot(ASFEnemyController* AIController, AActor* Target) const;

	/**
	 * 특정 AI의 모든 슬롯 강제 해제 (사망/소멸 시)
	 */
	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	void ReleaseAllSlots(ASFEnemyController* AIController);

	/**
	 * 멀티플레이어 그룹 찾기
	 */
	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	TArray<AActor*> GetPlayerGroup(AActor* Player) const;

protected:
	/** 타겟별 슬롯 정보 맵 */
	UPROPERTY()
	TMap<TObjectPtr<AActor>, FSFCombatSlotInfo> TargetSlots;

	/** 유효하지 않은 컨트롤러 정리 */
	void CleanupInvalidSlotHolders(FSFCombatSlotInfo& SlotInfo);

	/** 거리 기반 유효 최대 슬롯 수 계산 */
	int32 GetEffectiveMaxSlots(AActor* Target) const;
};