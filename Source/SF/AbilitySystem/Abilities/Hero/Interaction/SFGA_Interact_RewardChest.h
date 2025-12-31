#pragma once

#include "CoreMinimal.h"
#include "SFGA_Interact_Chest.h"
#include "SFGA_Interact_RewardChest.generated.h"

class USFStatBoostSelectionWidget;
struct FSFCommonUpgradeChoice;
class USFCommonLootTable;
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
	
	/**
	 *	스킬 업그레이드 (보스 스테이지)
	 */
	
	void ShowSkillUpgradeUI(int32 StageIndex);

	UFUNCTION()
	void OnSkillSelectionComplete();

private:
	
	/**
	 *	스탯 강화 (일반 스테이지)
	 */

	// 스탯 강화 선택 초기화 (서버/클라이언트 분기 처리) 
	void InitializeStatBoostSelection(int32 StageIndex);

	UFUNCTION()
	void OnClientReadyForStatBoost();
	
	// 선택지 수신 시 UI 표시 
	UFUNCTION()
	void OnStatBoostChoicesReceived(const TArray<FSFCommonUpgradeChoice>& Choices);

	// UI에서 카드 선택 시 
	UFUNCTION()
	void OnStatBoostCardSelected(int32 ChoiceIndex);

	// UI에서 종료 Delegate 호출시
	UFUNCTION()
	void OnStatBoostSelectionComplete();

	UFUNCTION()
	void OnStatBoostApplied();

	UFUNCTION()
	void OnStatBoostApplyFailed(const FText& Reason);

	UFUNCTION()
	void OnRerollFailed(const FText& Reason);

	void ShowStatBoostUI(const TArray<FSFCommonUpgradeChoice>& Choices);

private:
	void CleanupUI();
	void CleanupStatBoostDelegates();

protected:
	// 스킬 업그레이드 UI 위젯 클래스 
	UPROPERTY(EditDefaultsOnly, Category = "SF|Reward|SkillUpgrade")
	TSubclassOf<USFSkillSelectionScreen> SkillSelectionScreenClass;

	// 스탯 강화 UI 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "SF|Reward|StatBoost")
	TSubclassOf<USFStatBoostSelectionWidget> StatBoostWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<USFSkillSelectionScreen> SkillSelectionScreen;

	UPROPERTY()
	TObjectPtr<USFStatBoostSelectionWidget> StatBoostWidget;

	UPROPERTY()
	TWeakObjectPtr<ASFRewardChest> CachedRewardChest;

	bool bStatBoostDelegatesBound = false;

	int32 CachedStageIndex = 0;
};
