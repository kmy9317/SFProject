// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ControllerComponent.h"
#include "SFEnemyCombatComponent.generated.h"


struct FEnemyAbilitySelectContext;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFEnemyCombatComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	void InitializeCombatComponent();
	
	static USFEnemyCombatComponent* FindSFEnemyCombatComponent(const AController* Controller) { return (Controller ? Controller->FindComponentByClass<USFEnemyCombatComponent>() : nullptr); }
	
	bool SelectAbility(const FEnemyAbilitySelectContext& Context, const FGameplayTagContainer& SearchTags, FGameplayTag& OutSelectedTag);




};
