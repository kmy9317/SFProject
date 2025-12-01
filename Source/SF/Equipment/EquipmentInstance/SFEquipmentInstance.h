// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equipment/SFEquipmentDefinition.h"
#include "UObject/Object.h"
#include "SFEquipmentInstance.generated.h"

struct FGameplayAbilitySpecHandle;
struct FActiveGameplayEffectHandle;
class UAbilitySystemComponent;
class USFEquipmentDefinition;

USTRUCT(BlueprintType)
struct FSFEquipmentList
{
	GENERATED_BODY()
    
	FSFEquipmentList()
	{}
    
	//스폰된 Actor
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

public:

	void DestroySpawnedActors();

	int32 Num() const
	{
		return SpawnedActors.Num();
	}
};


UCLASS()
class SF_API USFEquipmentInstance : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(USFEquipmentDefinition* InDefinition, APawn* InPawn, UAbilitySystemComponent* ASC);
	void Deinitialize(UAbilitySystemComponent* ASC);
	

	UFUNCTION(BlueprintPure, Category = "Equipment")
	USFEquipmentDefinition* GetEquipmentDefinition() const { return EquipmentDefinition; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	APawn* GetInstigator() const { return Instigator; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	TArray<AActor*> GetSpawnedActors() const { return SpawnedActorList.SpawnedActors; }

protected:
	UPROPERTY()
	TObjectPtr<USFEquipmentDefinition> EquipmentDefinition;
    
	UPROPERTY()
	TObjectPtr<APawn> Instigator;
    
	UPROPERTY()
	FSFEquipmentList SpawnedActorList;
    
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
    
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> GrantedEffectHandles;

private:

	void SpawnEquipmentActors();
	void GrantAbilities(UAbilitySystemComponent* ASC);
	
	// Animation Layer 연결/해제
	void ApplyAnimationLayer();
	void RemoveAnimationLayer();
};