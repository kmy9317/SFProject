#pragma once

#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SFGameplayTagStack.generated.h"

struct FSFGameplayTagStackContainer;
struct FNetDeltaSerializeInfo;

USTRUCT(BlueprintType)
struct FSFGameplayTagStack : public FFastArraySerializerItem
{
	GENERATED_BODY()

public:
	FSFGameplayTagStack() { }

	FSFGameplayTagStack(FGameplayTag InTag, int32 InStackCount)
		: Tag(InTag)
		, StackCount(InStackCount) { }

public:
	const FGameplayTag& GetStackTag() const { return Tag; }
	int32 GetStackCount() const { return StackCount; }
	
	FString GetDebugString() const;
	
private:
	friend FSFGameplayTagStackContainer;

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	int32 StackCount = 0;
};

USTRUCT(BlueprintType)
struct FSFGameplayTagStackContainer : public FFastArraySerializer
{
	GENERATED_BODY()

public:
	FSFGameplayTagStackContainer() { }

public:
	void AddOrRemoveStack(FGameplayTag Tag, int32 StackCount);
	void AddStack(FGameplayTag Tag, int32 StackCount);
	void RemoveStack(FGameplayTag Tag, int32 StackCount);
	void RemoveStack(FGameplayTag Tag);

public:
	const TArray<FSFGameplayTagStack>& GetStacks() const { return Stacks; }
	FGameplayTagContainer GetTags() const { return TagContainer; }
	int32 GetStackCount(FGameplayTag Tag) const;
	bool ContainsTag(FGameplayTag Tag) const;
	
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FSFGameplayTagStack, FSFGameplayTagStackContainer>(Stacks, DeltaParms, *this);
	}

private:
	UPROPERTY()
	TArray<FSFGameplayTagStack> Stacks;
	
	TMap<FGameplayTag, int32> TagToCountMap;
	FGameplayTagContainer TagContainer;
};

template<>
struct TStructOpsTypeTraits<FSFGameplayTagStackContainer> : public TStructOpsTypeTraitsBase2<FSFGameplayTagStackContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
