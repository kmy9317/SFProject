#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/SFGA_Hero_Base.h"
#include "Item/SFItemManagerComponent.h"
#include "SFGA_Hero_Consume.generated.h"

class USFItemFragment_Consumable;
class USFItemInstance;
/**
 * 
 */
UCLASS()
class SF_API USFGA_Hero_Consume : public USFGA_Hero_Base
{
	GENERATED_BODY()

public:
	USFGA_Hero_Consume(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 슬롯 핸들 Getter (Cost에서 사용) 
	const FSFItemSlotHandle& GetConsumeSlotHandle() const { return ConsumeSlotHandle; }

	int32 GetConsumeCount() const;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// Fragment에서 Effect 적용
	UFUNCTION(BlueprintCallable, Category = "Consume")
	void ApplyConsumeEffect();

	UFUNCTION(BlueprintPure, Category = "Consume")
	USFItemInstance* GetConsumeItemInstance() const { return ConsumeItemInstance; }

	UFUNCTION(BlueprintPure, Category = "Consume")
	const USFItemFragment_Consumable* GetConsumeFragment() const { return ConsumeFragment; }

protected:
	// 실제 아이템 사용 여부 (EndAbility에서 소모 결정)
	UPROPERTY(BlueprintReadWrite, Category = "Consume")
	bool bItemUsed = false;

protected:
	UPROPERTY()
	FSFItemSlotHandle ConsumeSlotHandle;

private:
	UPROPERTY()
	TObjectPtr<USFItemInstance> ConsumeItemInstance;

	UPROPERTY()
	TObjectPtr<const USFItemFragment_Consumable> ConsumeFragment;
};
