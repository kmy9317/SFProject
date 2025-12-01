#include "SFEquipmentTypes.h"

#include "EquipmentInstance/SFEquipmentInstance.h"

void FSFEquipmentList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		const FSFAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance)
		{
			Entry.Instance->OnUnequipped();
		}
	}
}

void FSFEquipmentList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FSFAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance)
		{
			Entry.Instance->OnEquipped();
		}
	}
}

void FSFEquipmentList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	// TODO : 기존 Entry 구조체 멤버 변수가 변경될 때 호출(현재는 EquipmentInstance 변경시 호출)
}
