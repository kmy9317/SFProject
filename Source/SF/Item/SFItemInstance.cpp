#include "SFItemInstance.h"

#include "SFItemData.h"
#include "SFItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Player/Save/SFPersistentDataType.h"


USFItemInstance::USFItemInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
    
}

void USFItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ItemID);
	DOREPLIFETIME(ThisClass, ItemRarity);
	DOREPLIFETIME(ThisClass, StatContainer);
	DOREPLIFETIME(ThisClass, OwnedTagContainer);
}

void USFItemInstance::InitializeFromSavedData(const FSFSavedItemSlot& SavedSlot)
{
	ItemID = SavedSlot.ItemID;
	ItemRarity = SavedSlot.RarityTag;

	for (const FSFSavedTagStack& SavedStack : SavedSlot.StatStacks)
	{
		if (SavedStack.Tag.IsValid() && SavedStack.StackCount > 0)
		{
			StatContainer.AddStack(SavedStack.Tag, SavedStack.StackCount);
		}
	}

	for (const FSFSavedTagStack& SavedStack : SavedSlot.OwnedTagStacks)
	{
		if (SavedStack.Tag.IsValid() && SavedStack.StackCount > 0)
		{
			OwnedTagContainer.AddStack(SavedStack.Tag, SavedStack.StackCount);
		}
	}
}

void USFItemInstance::AddOrRemoveStatTagStack(FGameplayTag StatTag, int32 StackCount)
{
	StatContainer.AddOrRemoveStack(StatTag, StackCount);
}

void USFItemInstance::RemoveStatTagStack(FGameplayTag StatTag)
{
	StatContainer.RemoveStack(StatTag);
}

void USFItemInstance::AddOrRemoveOwnedTagStack(FGameplayTag OwnedTag, int32 StackCount)
{
	OwnedTagContainer.AddOrRemoveStack(OwnedTag, StackCount);
}

void USFItemInstance::RemoveOwnedTagStack(FGameplayTag OwnedTag)
{
	OwnedTagContainer.RemoveStack(OwnedTag);
}

bool USFItemInstance::HasStatTag(FGameplayTag StatTag) const
{
	return StatContainer.ContainsTag(StatTag);
}

int32 USFItemInstance::GetStatCountByTag(FGameplayTag StatTag) const
{
	return StatContainer.GetStackCount(StatTag);
}

bool USFItemInstance::HasOwnedTag(FGameplayTag OwnedTag) const
{
	return OwnedTagContainer.ContainsTag(OwnedTag);
}

int32 USFItemInstance::GetOwnedCountByTag(FGameplayTag OwnedTag) const
{
	return OwnedTagContainer.GetStackCount(OwnedTag);
}

const USFItemFragment* USFItemInstance::FindFragmentByClass(TSubclassOf<USFItemFragment> FragmentClass) const
{
	if (ItemID > INDEX_NONE && FragmentClass)
	{
		if (const USFItemDefinition* ItemDefinition = USFItemData::Get().FindDefinitionById(ItemID))
		{
			return ItemDefinition->FindFragmentByClass(FragmentClass);
		}
	}
	return nullptr;
}
