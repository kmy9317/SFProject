// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equipment/SFEquipmentDefinition.h"
#include "UObject/Object.h"
#include "SFEquipmentInstance.generated.h"

class USFEquipmentComponent;
struct FGameplayAbilitySpecHandle;
struct FActiveGameplayEffectHandle;
class UAbilitySystemComponent;
class USFEquipmentDefinition;

UCLASS()
class SF_API USFEquipmentInstance : public UObject
{
	GENERATED_BODY()

public:

	USFEquipmentInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void Initialize(USFEquipmentDefinition* InDefinition, APawn* InPawn, UAbilitySystemComponent* ASC);
	void Deinitialize(UAbilitySystemComponent* ASC);

	UFUNCTION(BlueprintPure, Category = "Equipment")
	USFEquipmentDefinition* GetEquipmentDefinition() const { return EquipmentDefinition; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	APawn* GetInstigator() const { return Instigator; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	TArray<AActor*> GetSpawnedActors() const { return SpawnedActors; }

	// 리플리케이션 콜백
	void OnEquipped();
	void OnUnequipped();

protected:
	UFUNCTION()
	void OnRep_EquipmentDefinition();

	UFUNCTION()
	void OnRep_Instigator();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_EquipmentDefinition)
	TObjectPtr<USFEquipmentDefinition> EquipmentDefinition;
    
	UPROPERTY(ReplicatedUsing = OnRep_Instigator)
	TObjectPtr<APawn> Instigator;
    
	UPROPERTY(Replicated)
	TArray<TObjectPtr<AActor>> SpawnedActors;
    
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
    
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> GrantedEffectHandles;

private:

	void SpawnEquipmentActors();
	void DestroyEquipmentActors();
	void GrantAbilities(UAbilitySystemComponent* ASC);
	void RemoveAbilities(UAbilitySystemComponent* ASC);
	
	// Animation Layer 연결/해제
	void ApplyAnimationLayer();
	void RemoveAnimationLayer();
};