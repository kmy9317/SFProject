// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"
#include "Character/SFPawnData.h"
#include "SFEnemyData.generated.h"

class USFState;
class UBehaviorTree;

USTRUCT(BlueprintType)
struct FSFBehaviourWrapper
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag BehaviourTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UBehaviorTree> Behaviour;
};

USTRUCT(BlueprintType)
struct FSFBehaviourWrapperContainer
{
	GENERATED_BODY()
	
	UBehaviorTree* GetBehaviourTree(const FGameplayTag& Tag) const;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FSFBehaviourWrapper> Behaviours;
};

USTRUCT(BlueprintType)
struct FSTStateWrapper
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag StateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<USFState> StateClass;
};

USTRUCT(BlueprintType)
struct FSTStateWrapperContainer
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FSTStateWrapper> States;
};

USTRUCT(BlueprintType)
struct FSTTaggedMontageContainer
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FTaggedMontage> Container;

	UAnimMontage* GetMontage(const FGameplayTag& Tag) const;
};

/**
 * 
 */
UCLASS()
class SF_API USFEnemyData : public USFPawnData
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|ID")
	FName EnemyID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Type")
	FGameplayTag EnemyType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behaviour")
	FSFBehaviourWrapperContainer BehaviourContainer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|State")
	FGameplayTag DefaultBehaviourTag;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|State")
	FSTStateWrapperContainer StateContainer;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|State")
	FGameplayTag DefaultStateTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Montage")
	FSTTaggedMontageContainer MontageContainer;

	// Turn In Place 설정
	// true: Turn 애니메이션이 RootMotion을 가지고 있음 (잡몹)
	// false: Turn 애니메이션이 In-Place이며 AI Controller가 회전시킴 (보스)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Animation")
	bool bUseTurnInPlaceRootMotion = true;


};