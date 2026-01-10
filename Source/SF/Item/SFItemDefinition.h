#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "SFItemDefinition.generated.h"

class UAbilitySystemComponent;
class USFItemInstance;

UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class USFItemFragment : public UObject
{
	GENERATED_BODY()
	
public:
	// Instance 생성 시 초기화 (스탯 롤링 등)
	virtual void OnInstanceCreated(USFItemInstance* Instance) const { }

	// 장착 시 효과 적용
	virtual void OnEquipped(UAbilitySystemComponent* ASC, USFItemInstance* Instance) const {}

	// 장착 해제 시 효과 제거
	virtual void OnUnequipped(UAbilitySystemComponent* ASC, USFItemInstance* Instance) const {}

	// 사용 시 (소모품)
	virtual bool OnUsed(UAbilitySystemComponent* ASC, USFItemInstance* Instance) const { return false; }
};

UCLASS(Blueprintable, Const, Abstract)
class SF_API USFItemDefinition : public UObject
{
	GENERATED_BODY()

public:
	USFItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	

protected:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif // WITH_EDITOR

public:
	UFUNCTION(BlueprintCallable, BlueprintPure="false", meta=(DeterminesOutputType="FragmentClass"))
	const USFItemFragment* FindFragmentByClass(TSubclassOf<USFItemFragment> FragmentClass) const;

	template <typename T>
	const T* FindFragment() const
	{
		return Cast<T>(FindFragmentByClass(T::StaticClass()));
	}

	bool CanDropWithRarity(const FGameplayTag& RarityTag) const;

public:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText DisplayName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText Description;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	UPROPERTY(EditDefaultsOnly)
	int32 MaxStackCount = 1;

	// 등급 제한(비어있으면 모든 등급 허용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rarity", meta = (Categories = "Rarity"))
	FGameplayTagContainer AllowedRarities;

	// 드롭 시 개별 스폰 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop")
	bool bSpawnIndividually = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced)
	TArray<TObjectPtr<USFItemFragment>> Fragments;
};
