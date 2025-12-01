#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/PawnComponent.h"
#include "Equipment/SFEquipmentTypes.h"
#include "SFEquipmentComponent.generated.h"

class USFAbilitySystemComponent;
class ASFCharacterBase;
class USFEquipmentInstance;
class USFEquipmentDefinition;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFEquipmentComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	USFEquipmentComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void ReadyForReplication() override;
	
	static USFEquipmentComponent* FindEquipmentComponent(AActor* OwnerActor){ return (OwnerActor ? OwnerActor->FindComponentByClass<USFEquipmentComponent>() : nullptr); }

	void EquipItem(USFEquipmentDefinition* EquipmentDefinition);

	// 슬롯 태그로 장비 제거
	void UnequipItem(FGameplayTag EquipmentSlotTag);

	// Instance로 직접 장비 제거
	void UnequipItemByInstance(USFEquipmentInstance* EquipmentInstance);
	
	UFUNCTION(BlueprintPure, Category = "Equipment")
	TArray<USFEquipmentInstance*> GetEquippedItems() const;
	
	UFUNCTION(BlueprintPure, Category = "Equipment")
	AActor* GetFirstEquippedActorBySlot(const FGameplayTag& SlotTag) const;

	void InitializeEquipment();

	virtual USFEquipmentInstance* FindEquipmentInstance(FGameplayTag EquipmentTag) const;

	virtual USFEquipmentInstance* FindEquipmentInstanceBySlot(FGameplayTag SlotTag) const;

	bool IsSlotEquipmentMatchesTag(const FGameplayTag& SlotTag, const FGameplayTag& CheckingTag) const;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void OnAbilitySystemInitialized();
	void OnAbilitySystemUninitialized();

	void UninitializeAllEquipment();
	USFAbilitySystemComponent* GetAbilitySystemComponent() const;

protected:
	friend struct FSFEquipmentList;

	UPROPERTY(Replicated)
	FSFEquipmentList EquipmentList;

	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TArray<TObjectPtr<USFEquipmentDefinition>> DefaultEquipmentDefinitions;

	uint8 bEquipmentInitialized : 1;
};