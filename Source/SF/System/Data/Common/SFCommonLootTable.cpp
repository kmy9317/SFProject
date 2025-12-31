#include "SFCommonLootTable.h"

void USFCommonLootTable::PostLoad()
{
	Super::PostLoad();
	RecalculateTotalWeight();
}

#if WITH_EDITOR
void USFCommonLootTable::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RecalculateTotalWeight();
}
#endif

void USFCommonLootTable::RecalculateTotalWeight()
{
	CachedTotalWeight = 0.0f;
	for (const FSFCommonLootEntry& Entry : LootEntries)
	{
		// 0 이하의 가중치는 계산에서 제외 
		if (Entry.Weight > 0.0f)
		{
			CachedTotalWeight += Entry.Weight;
		}
	}
}
