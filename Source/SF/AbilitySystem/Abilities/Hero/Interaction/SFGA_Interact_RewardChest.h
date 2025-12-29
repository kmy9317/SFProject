#pragma once

#include "CoreMinimal.h"
#include "SFGA_Interact_Chest.h"
#include "SFGA_Interact_RewardChest.generated.h"

/**
 * 보상 상자 전용 어빌리티
 * - 보스 스테이지: 스킬 업그레이드 UI
 * - 일반 스테이지: 스탯 강화 UI (추후 구현)
 */
UCLASS()
class SF_API USFGA_Interact_RewardChest : public USFGA_Interact_Chest
{
	GENERATED_BODY()

public:
	USFGA_Interact_RewardChest(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void OnChestOpened(ASFChestBase* ChestActor) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	// 스킬 업그레이드 UI 표시 (보스 스테이지)
	void ShowSkillUpgradeUI(int32 StageIndex);

	// 스탯 강화 UI 표시 (일반 스테이지) 
	void ShowStatBoostUI(int32 StageIndex);

	void CleanupUI();

	UFUNCTION()
	void OnSkillSelectionComplete();

	UFUNCTION()
	void OnStatBoostSelectionComplete();

protected:
	// 스킬 업그레이드 UI 위젯 클래스 
	UPROPERTY(EditDefaultsOnly, Category = "SF|Reward|SkillUpgrade")
	TSubclassOf<USFSkillSelectionScreen> SkillSelectionScreenClass;

	// TODO : 스탯 강화 UI 위젯 클래스로 추후 지정 예정 
	UPROPERTY(EditDefaultsOnly, Category = "SF|Reward|StatBoost")
	TSubclassOf<UUserWidget> StatBoostWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<USFSkillSelectionScreen> SkillSelectionScreen;

	UPROPERTY()
	TObjectPtr<UUserWidget> StatBoostWidget;

	UPROPERTY()
	TWeakObjectPtr<ASFRewardChest> CachedRewardChest;
};
