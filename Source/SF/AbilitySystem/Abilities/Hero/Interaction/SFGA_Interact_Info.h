// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "Interaction/SFInteractionInfo.h"
#include "SFGA_Interact_Info.generated.h"

class ISFInteractable;
/**
 * 
 */
UCLASS()
class SF_API USFGA_Interact_Info : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Interact_Info(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	UFUNCTION(BlueprintCallable)
	bool InitializeAbility(AActor* TargetActor);

protected:
	UPROPERTY(BlueprintReadOnly)
	TScriptInterface<ISFInteractable> Interactable;
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> InteractableActor;
	
	UPROPERTY(BlueprintReadOnly)
	FSFInteractionInfo InteractionInfo;
};
