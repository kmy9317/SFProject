// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SFPawnData.generated.h"

class USFGameplayAbility;
class USFInputConfig;
class USFAbilitySet;
class USFCameraMode;

USTRUCT(BlueprintType)
struct FSFSkillUpgradeOptionList
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSubclassOf<USFGameplayAbility>> UpgradeAbilities;
};

/**
 * 
 */
UCLASS()
class SF_API USFPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static FPrimaryAssetType GetPawnAssetType() { return FPrimaryAssetType(TEXT("PawnData")); }

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(GetPawnAssetType(), GetFName());
	}

	USFPawnData(const FObjectInitializer& ObjectInitializer);

	// 특정 InputTag의 업그레이드 선택지 반환
	UFUNCTION(BlueprintCallable, Category = "SF|SkillUpgrade")
	TArray<TSubclassOf<USFGameplayAbility>> GetUpgradeOptionsForSlot(FGameplayTag InputTag) const;
    
	// 스테이지 인덱스에 해당하는 InputTag 반환
	UFUNCTION(BlueprintCallable, Category = "SF|SkillUpgrade")
	FGameplayTag GetUpgradeSlotTagForStage(int32 StageIndex) const;

public:

	// Class to instantiate for this pawn (should usually derive from ALCPawn or ALCCharacter).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Pawn")
	TSubclassOf<APawn> PawnClass;

	// Ability sets to grant to this pawn's ability system.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Abilities")
	TArray<TObjectPtr<USFAbilitySet>> AbilitySets;

	// Input configuration used by player controlled pawns to create input mappings and bind input actions.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Input")
	TObjectPtr<USFInputConfig> InputConfig;

	// Default camera mode used by player controlled pawns.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Camera")
	TSubclassOf<USFCameraMode> DefaultCameraMode;

	// 카테고리별 업그레이드 선택지 (EX : InputTag.PrimarySkill → Primary skills[GA_A, GA_B, GA_C])
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|SkillUpgrade", meta = (Categories = "InputTag"))
	TMap<FGameplayTag, FSFSkillUpgradeOptionList> SkillUpgradeMap;
    
	// 스테이지별 업그레이드할 카테고리 순서
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|SkillUpgrade", meta = (Categories = "InputTag"))
	TArray<FGameplayTag> UpgradeSlotOrder;

	
};
