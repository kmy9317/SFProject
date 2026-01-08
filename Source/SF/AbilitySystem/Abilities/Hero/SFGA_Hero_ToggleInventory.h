// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Hero_ToggleInventory.generated.h"

class USFInventoryScreenWidget;
/**
 * 
 */
UCLASS()
class SF_API USFGA_Hero_ToggleInventory : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Hero_ToggleInventory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** Tab 재입력 시 닫기 */
	void OnToggleInputReceived(const FGameplayEventData* Payload);

	/** 위젯에서 닫기 요청 시 */
	UFUNCTION()
	void OnScreenClosed();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USFInventoryScreenWidget> InventoryScreenWidgetClass;

private:

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	int32 InventoryScreenZOrder = 80;
	
	UPROPERTY()
	TObjectPtr<USFInventoryScreenWidget> InventoryScreenWidget;

	FDelegateHandle InputEventHandle;
};
