#pragma once

#include "CoreMinimal.h"
#include "System/SFGameplayTagStack.h"
#include "UObject/Object.h"
#include "SFItemInstance.generated.h"

class USFItemDefinition;
class USFItemFragment;
class USFItemData;

UCLASS()
class SF_API USFItemInstance : public UObject
{
	GENERATED_BODY()
public:
	USFItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override { return true; }

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddOrRemoveStatTagStack(FGameplayTag StatTag, int32 StackCount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveStatTagStack(FGameplayTag StatTag);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddOrRemoveOwnedTagStack(FGameplayTag OwnedTag, int32 StackCount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveOwnedTagStack(FGameplayTag OwnedTag);

public:
	UFUNCTION(BlueprintCallable)
	int32 GetItemID() const { return ItemID; }
	
	UFUNCTION(BlueprintCallable)
	FGameplayTag GetItemRarityTag() const { return ItemRarity; }
	
	UFUNCTION(BlueprintCallable)
	bool HasStatTag(FGameplayTag StatTag) const;

	UFUNCTION(BlueprintCallable)
	int32 GetStatCountByTag(FGameplayTag StatTag) const;

	UFUNCTION(BlueprintCallable)
	const FSFGameplayTagStackContainer& GetStatContainer() const { return StatContainer; }

	UFUNCTION(BlueprintCallable)
	bool HasOwnedTag(FGameplayTag OwnedTag) const;

	UFUNCTION(BlueprintCallable)
	int32 GetOwnedCountByTag(FGameplayTag OwnedTag) const;
	
	UFUNCTION(BlueprintCallable)
	const FSFGameplayTagStackContainer& GetOwnedTagContainer() const { return OwnedTagContainer; }
	
public:
	UFUNCTION(BlueprintCallable, BlueprintPure="false", meta=(DeterminesOutputType="FragmentClass"))
	const USFItemFragment* FindFragmentByClass(TSubclassOf<USFItemFragment> FragmentClass) const;

	template <typename T>
	const T* FindFragmentByClass() const
	{
		return Cast<T>(FindFragmentByClass(T::StaticClass()));
	}

private:
	friend class USFItemData;
	
	UPROPERTY(Replicated)
	int32 ItemID = INDEX_NONE;

	UPROPERTY(Replicated)
	FGameplayTag ItemRarity;

	// 고정 스탯 (공격력, 방어력 등)
	UPROPERTY(Replicated)
	FSFGameplayTagStackContainer StatContainer;

	// 동적 상태 (장전된 화살, 내구도 등)
	UPROPERTY(Replicated)
	FSFGameplayTagStackContainer OwnedTagContainer;
};
