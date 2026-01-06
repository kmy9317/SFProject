#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFDropTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SFDropFunctionLibrary.generated.h"

class USFInventoryManagerComponent;
/**
 * 
 */
UCLASS()
class SF_API USFDropFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 드롭 결과만 생성 (스폰 안 함)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Drop")
	static TArray<FSFDropResult> GenerateDropResults(UObject* Outer, const USFDropTable* DropTable, float LuckValue);

	// 결과를 월드에 스폰
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Drop", meta = (WorldContext = "WorldContextObject"))
	static void SpawnDropResults(UObject* WorldContextObject,const TArray<FSFDropResult>& DropResults, const FVector& Location, float SpawnRadius = 100.f);

	// 생성 + 스폰 (적 드롭용)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Drop", meta = (WorldContext = "WorldContextObject"))
	static void ExecuteAndSpawnDrops(UObject* WorldContextObject, const USFDropTable* DropTable, float LuckValue, const FVector& Location, float SpawnRadius = 100.f);

	// 결과를 인벤토리에 직접 추가 (상자 UI용)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Drop")
	static bool AddDropResultsToInventory(USFInventoryManagerComponent* InventoryManager, const TArray<FSFDropResult>& DropResults);

private:
	// 등급 결정 헬퍼
	static FGameplayTag DetermineRarity(const FSFDropTableEntry& Entry, float LuckValue);
};
