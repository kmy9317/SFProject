#pragma once

#include "CoreMinimal.h"
#include "SFChestBase.h"
#include "SFRewardChest.generated.h"

struct FSFStageInfo;
/**
 * 보상 상자의 보상 타입
 * 스테이지 타입에 따라 자동 결정
 */
UENUM(BlueprintType)
enum class ESFRewardChestType : uint8
{
	None,
	SkillUpgrade,    // 보스 스테이지 - 스킬 진화
	StatBoost,       // 일반 스테이지 - 등급별 스탯 강화
					// TODO : 휴식 스테이지 상자 보상 타입 지정 가능
};

UCLASS()
class SF_API ASFRewardChest : public ASFChestBase
{
	GENERATED_BODY()

public:
	ASFRewardChest(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	
	// ~ Begin ISFInteractable
	virtual bool CanInteraction(const FSFInteractionQuery& InteractionQuery) const override;
	// ~ End ISFInteractable

	// ~ Begin ASFWorldInteractable
	virtual void OnInteractionSuccess(AActor* Interactor) override;
	// ~ End ASFWorldInteractable

	UFUNCTION(BlueprintPure, Category = "SF|Reward")
	ESFRewardChestType GetRewardType() const { return RewardType; }

	UFUNCTION(BlueprintPure, Category = "SF|Reward")
	int32 GetStageIndex() const { return CachedStageIndex; }

	UFUNCTION(BlueprintPure, Category = "SF|Reward")
	bool IsActivated() const { return bIsActivated; }

	UFUNCTION(BlueprintPure, Category = "SF|Reward")
	bool HasPlayerClaimed(APlayerState* PlayerState) const;

protected:
	UFUNCTION()
	void OnStageClearedHandler(const FSFStageInfo& ClearedStageInfo);

	virtual ESFRewardChestType DetermineRewardType(const FSFStageInfo& StageInfo) const;

	UFUNCTION()
	void OnRep_RewardType();

	UFUNCTION()
	void OnRep_IsActivated();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_RewardType, BlueprintReadOnly, Category = "SF|Reward")
	ESFRewardChestType RewardType = ESFRewardChestType::None;

	UPROPERTY(ReplicatedUsing = OnRep_IsActivated, BlueprintReadOnly, Category = "SF|Reward")
	bool bIsActivated = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "SF|Reward")
	int32 CachedStageIndex = 0;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<APlayerState>> ClaimedPlayers;

	// 에디터에서 보상 타입 강제 지정 (테스트용) 
	UPROPERTY(EditAnywhere, Category = "SF|Reward|Debug")
	bool bOverrideRewardType = false;

	UPROPERTY(EditAnywhere, Category = "SF|Reward|Debug", meta = (EditCondition = "bOverrideRewardType"))
	ESFRewardChestType OverriddenRewardType = ESFRewardChestType::SkillUpgrade;
};
