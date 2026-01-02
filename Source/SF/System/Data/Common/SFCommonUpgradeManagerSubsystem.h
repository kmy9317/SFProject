#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "SFCommonUpgradeManagerSubsystem.generated.h"

class USFCommonUpgradeFragment_StatBoost;
class USFCommonUpgradeFragment_SkillLevel;
class UAbilitySystemComponent;
struct FSFCommonUpgradeChoice;
class USFCommonRarityConfig;
class USFCommonUpgradeDefinition;
class ASFPlayerState;
class USFCommonLootTable;

/**
 * 플레이어별 리롤 상태를 추적하기 위한 컨텍스트
 */
USTRUCT()
struct FSFCommonUpgradeContext
{
	GENERATED_BODY()

	// 리롤 시 동일한 테이블을 사용하기 위해 저장
	UPROPERTY()
	TObjectPtr<USFCommonLootTable> SourceLootTable;

	UPROPERTY()
	int32 SlotCount = 3;

	UPROPERTY()
	int32 RerollCount = 0;

	// 무료 리롤 사용 여부 (상자당 1회)
	UPROPERTY()
	bool bUsedFreeReroll = false;

	// 추가 선택 사용 여부 (상자당 1회)
	UPROPERTY()
	bool bUsedMoreEnhance = false;
	
	// 서버 검증용: 생성된 선택지들
	UPROPERTY()
	TArray<FSFCommonUpgradeChoice> PendingChoices;

	void Reset()
	{
		SourceLootTable = nullptr;
		SlotCount = 3;
		RerollCount = 0;
		bUsedFreeReroll = false;
		bUsedMoreEnhance = false;
		PendingChoices.Empty();
	}
};

/**
 * 일반 강화 및 리롤 로직을 담당하는 서버 전용 서브시스템
 */
UCLASS()
class SF_API USFCommonUpgradeManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// 상자 상호작용 시 호출. 보상 선택지 3개를 생성
	UFUNCTION(BlueprintCallable, Category = "SF|Upgrade")
	TArray<FSFCommonUpgradeChoice> GenerateUpgradeOptions(ASFPlayerState* PlayerState, USFCommonLootTable* LootTable, int32 Count = 3);
	
	// UI에서 리롤 버튼 클릭 시 호출. 재화(Tag)를 소모하고 선택지를 재생성
	UFUNCTION(BlueprintCallable, Category = "SF|Upgrade")
	TArray<FSFCommonUpgradeChoice> TryRerollOptions(ASFPlayerState* PlayerState);

	// 플레이어가 선택한 업그레이드 적용 (UniqueId 기반)
	UFUNCTION(BlueprintCallable, Category = "SF|Upgrade")
	bool ApplyUpgradeChoice(ASFPlayerState* PlayerState, const FGuid& ChoiceId);

	// 플레이어가 선택한 업그레이드 적용 (Index 기반)
	UFUNCTION(BlueprintCallable, Category = "SF|Upgrade")
	bool ApplyUpgradeChoiceByIndex(ASFPlayerState* PlayerState, int32 ChoiceIndex);

	// 리롤 비용 계산 (FreeReroll 태그 고려)
	UFUNCTION(BlueprintCallable, Category = "SF|Upgrade")
	int32 CalculateRerollCost(ASFPlayerState* PlayerState) const;

	// 리롤 가능 여부 (골드 또는 FreeReroll 태그)
	UFUNCTION(BlueprintCallable, Category = "SF|Upgrade")
	bool CanReroll(ASFPlayerState* PlayerState) const;

	// 추가 선택 가능 여부 (MoreEnhance 태그)
	UFUNCTION(BlueprintCallable, Category = "SF|Upgrade")
	bool HasMoreEnhanceAvailable(ASFPlayerState* PlayerState) const;

	void ClearUpgradeContext(ASFPlayerState* PlayerState);
	
protected:
	void CacheCoreData();
	float GetPlayerLuck(ASFPlayerState* PlayerState) const;
	USFCommonUpgradeDefinition* PickRandomUpgrade(const USFCommonLootTable* Table, const TSet<USFCommonUpgradeDefinition*>& ExcludedItems, const FGameplayTag& RarityTag);
	USFCommonRarityConfig* PickRandomRarity(float LuckValue);

	void ApplyStatBoostFragment(UAbilitySystemComponent* ASC, const USFCommonUpgradeFragment_StatBoost* Fragment, float FinalMagnitude);
	void ApplySkillLevelFragment(UAbilitySystemComponent* ASC, const USFCommonUpgradeFragment_SkillLevel* Fragment);

	int32 GetRerollCostByCount(int32 RerollCount) const;

protected:
	// 플레이어별 업그레이드 컨텍스트
	UPROPERTY(Transient)
	TMap<TObjectPtr<ASFPlayerState>, FSFCommonUpgradeContext> ActiveUpgradeContexts;
	
	// 자주 쓰이는 Rarity Config를 미리 로드
	UPROPERTY(Transient)
	TArray<TObjectPtr<USFCommonRarityConfig>> CachedRarityConfigs;

	// 리롤 비용 태그
	FGameplayTag RerollCostTag;
};
