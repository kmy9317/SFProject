#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "SFPersistentDataType.generated.h"

class UGameplayEffect;
class UGameplayAbility;

USTRUCT()
struct FSFSavedAttribute
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayAttribute Attribute;

	//Base Value만 저장 (Current Value는 버프 재적용으로 자동 계산됨)
	UPROPERTY()
	float BaseValue = 0.f;
};

USTRUCT()
struct FSFSavedAbility
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY()
	int32 AbilityLevel = 1;

	UPROPERTY()
	FGameplayTagContainer DynamicTags;
};

USTRUCT()
struct FSFSavedGameplayEffect
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY()
	float EffectLevel = 1.f;

	// 남은 시간 (절대 시간이 아닌 상대적 시간)
	UPROPERTY()
	float RemainingDuration = 0.f;

	UPROPERTY()
	int32 StackCount = 1;
};

USTRUCT()
struct FSFSavedAbilitySystemData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FSFSavedAttribute> SavedAttributes;

	UPROPERTY()
	TArray<FSFSavedAbility> SavedAbilities;

	UPROPERTY()
	TArray<FSFSavedGameplayEffect> SavedGameplayEffects;

	bool HasSavedAttributes() const { return SavedAttributes.Num() > 0; }
	bool HasSavedAbilities() const { return SavedAbilities.Num() > 0; }
	bool HasSavedEffects() const { return SavedGameplayEffects.Num() > 0; }
	bool IsValid() const { return HasSavedAttributes() || HasSavedAbilities() || HasSavedEffects(); }

	void Reset()
	{
		SavedAttributes.Reset();
		SavedAbilities.Reset();
		SavedGameplayEffects.Reset();
	}
};

USTRUCT()
struct FSFSavedTagStack
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	int32 StackCount = 0;
};

USTRUCT()
struct FSFSavedItemSlot
{
	GENERATED_BODY()

	UPROPERTY()
	int32 ItemID = INDEX_NONE;

	UPROPERTY()
	FGameplayTag RarityTag;

	UPROPERTY()
	int32 ItemCount = 0;

	UPROPERTY()
	TArray<FSFSavedTagStack> StatStacks;

	UPROPERTY()
	TArray<FSFSavedTagStack> OwnedTagStacks;

	bool IsValid() const { return ItemID > INDEX_NONE && ItemCount > 0; }
};

USTRUCT()
struct FSFSavedInventoryData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FSFSavedItemSlot> InventorySlots;

	UPROPERTY()
	TArray<FSFSavedItemSlot> QuickbarSlots;

	bool IsValid() const
	{
		for (const FSFSavedItemSlot& Slot : InventorySlots)
		{
			if (Slot.IsValid())
			{
				return true;
			}
		}
		for (const FSFSavedItemSlot& Slot : QuickbarSlots)
		{
			if (Slot.IsValid())
			{
				return true;
			}
		}
		return false;
	}

	void Reset()
	{
		InventorySlots.Empty();
		QuickbarSlots.Empty();
	}
};
