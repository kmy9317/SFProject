#include "SFWorldPickupable.h"

#include "Engine/ActorChannel.h"
#include "Item/SFItemData.h"
#include "Item/SFItemDefinition.h"
#include "Item/SFItemInstance.h"
#include "Net/UnrealNetwork.h"

ASFWorldPickupable::ASFWorldPickupable(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
}

void ASFWorldPickupable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PickupInfo);
}

bool ASFWorldPickupable::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	if (USFItemInstance* ItemInstance = PickupInfo.PickupInstance.ItemInstance)
	{
		bWroteSomething |= Channel->ReplicateSubobject(ItemInstance, *Bunch, *RepFlags);
	}

	return bWroteSomething;
}

void ASFWorldPickupable::SetPickupInfo(const FSFPickupInfo& InPickupInfo)
{
	if (!HasAuthority())
	{
		return;
	}
	if (InPickupInfo.PickupInstance.ItemInstance || InPickupInfo.PickupDefinition.ItemDefinitionClass)
	{
		PickupInfo = InPickupInfo;
		OnRep_PickupInfo();
	}
	else
	{
		Destroy();
	}
}

void ASFWorldPickupable::OnRep_PickupInfo()
{
	if (const USFItemInstance* ItemInstance = PickupInfo.PickupInstance.ItemInstance)
	{
		if (const USFItemDefinition* ItemDefinition = USFItemData::Get().FindDefinitionById(ItemInstance->GetItemID()))
		{
			InteractionInfo.Content = ItemDefinition->DisplayName;
		}
	}
	else if (TSubclassOf<USFItemDefinition> ItemDefinitionClass = PickupInfo.PickupDefinition.ItemDefinitionClass)
	{
		if (const USFItemDefinition* ItemDefinition = ItemDefinitionClass->GetDefaultObject<USFItemDefinition>())
		{
			InteractionInfo.Content = ItemDefinition->DisplayName;
		}
	}
}


