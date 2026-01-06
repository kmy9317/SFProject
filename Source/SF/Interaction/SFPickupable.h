#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "System/Data/Common/SFCommonUpgradeGameplayTags.h"
#include "UObject/Interface.h"
#include "SFPickupable.generated.h"

class USFItemDefinition;
class USFItemInstance;

USTRUCT(BlueprintType)
struct FSFPickupDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<USFItemDefinition> ItemDefinitionClass;

	UPROPERTY(EditAnywhere)
	FGameplayTag RarityTag = SFGameplayTags::Rarity_Common;
	
	UPROPERTY(EditAnywhere)
	int32 ItemCount = 1;
};

USTRUCT(BlueprintType)
struct FSFPickupInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TObjectPtr<USFItemInstance> ItemInstance;

	UPROPERTY(EditAnywhere)
	int32 ItemCount = 1;
};

USTRUCT(BlueprintType)
struct FSFPickupInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FSFPickupDefinition PickupDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FSFPickupInstance PickupInstance;
};

UINTERFACE(MinimalAPI, BlueprintType, meta=(CannotImplementInterfaceInBlueprint))
class USFPickupable : public UInterface
{
	GENERATED_BODY()
};

class SF_API ISFPickupable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	virtual FSFPickupInfo GetPickupInfo() const = 0;
};
