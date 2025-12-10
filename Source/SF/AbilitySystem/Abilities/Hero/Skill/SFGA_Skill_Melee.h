// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/SFGA_Equipment_Base.h"
#include "SFGA_Skill_Melee.generated.h"

class ASFEquipmentBase;
/**
 * 
 */
UCLASS()
class SF_API USFGA_Skill_Melee : public USFGA_Equipment_Base
{
	GENERATED_BODY()

public:
	USFGA_Skill_Melee(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	void ParseTargetData(const FGameplayAbilityTargetDataHandle& InTargetDataHandle, TArray<int32>& OutActorsHitIndexes);
	
	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	void ProcessHitResult(FHitResult HitResult, float Damage, ASFEquipmentBase* WeaponActor);

	UFUNCTION(BlueprintCallable, Category = "SF|Ability") 
	void ResetHitActors();

	void DrawDebugHitPoint(const FHitResult& HitResult);
	
protected:

	UPROPERTY(EditDefaultsOnly, Category = "SF|Debug")
	bool bShowDebug = false;
	
	UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> CachedHitActors;
};
