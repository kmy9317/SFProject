#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SFEquipmentTypes.generated.h"

struct FSFEquipmentList;
class USFEquipmentDefinition;
class USFEquipmentInstance;
class USFEquipmentComponent;

USTRUCT(BlueprintType)
struct FSFAppliedEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FSFAppliedEquipmentEntry() = default;

private:
	friend USFEquipmentComponent;
	friend FSFEquipmentList;
	
	UPROPERTY()
	TObjectPtr<USFEquipmentInstance> Instance = nullptr;

};

USTRUCT(BlueprintType)
struct FSFEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	FSFEquipmentList() : OwnerComponent(nullptr) {}
	FSFEquipmentList(USFEquipmentComponent* InOwnerComponent) : OwnerComponent(InOwnerComponent) {}

	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FSFAppliedEquipmentEntry, FSFEquipmentList>(Entries, DeltaParms, *this);
	}

private:
	friend USFEquipmentComponent;

	UPROPERTY()
	TArray<FSFAppliedEquipmentEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<USFEquipmentComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FSFEquipmentList> : public TStructOpsTypeTraitsBase2<FSFEquipmentList>
{
	enum { WithNetDeltaSerializer = true };
};