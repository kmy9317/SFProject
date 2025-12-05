// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SFCombatSlotManager.generated.h"

class ASFEnemyController;

USTRUCT()
struct FSFCombatSlotInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TWeakObjectPtr<ASFEnemyController>> SlotHolders;

	double CombatStartTime = 0.0f;
	int32 BaseMaxSlots = 5;
};

UCLASS()
class SF_API USFCombatSlotManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SF|Combat")
	float PlayerGroupDistance = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SF|Combat")
	float CombatTempoScale = 10.0f;

public:
	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	bool RequestSlot(ASFEnemyController* Requester, AActor* Target, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	void ReleaseSlot(ASFEnemyController* Releaser, AActor* Target = nullptr);

	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	bool GetSlotInfo(AActor* Target, int32& OutCurrentSlots, int32& OutMaxSlots) const;

	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	bool HasSlot(ASFEnemyController* AIController, AActor* Target) const;

	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	void ReleaseAllSlots(ASFEnemyController* AIController);

protected:
	/** * [중요] UPROPERTY 매크로 제거됨 
	 * TMap의 Key로 TWeakObjectPtr를 쓸 때는 UPROPERTY를 붙이면 크래시가 발생합니다.
	 */
	TMap<TWeakObjectPtr<AActor>, FSFCombatSlotInfo> TargetSlots;

	int32 CalculateMaxSlotsForTarget(AActor* Target, const FSFCombatSlotInfo& SlotInfo) const;

	int32 CleanupAndCountSlots(FSFCombatSlotInfo& SlotInfo);
};