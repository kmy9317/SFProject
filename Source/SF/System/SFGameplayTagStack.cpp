#include "SFGameplayTagStack.h"

#include "UObject/Stack.h"

FString FSFGameplayTagStack::GetDebugString() const
{
	return FString::Printf(TEXT("%sx%d"), *Tag.ToString(), StackCount);
}

void FSFGameplayTagStackContainer::AddOrRemoveStack(FGameplayTag Tag, int32 StackCount)
{
	if (Tag.IsValid() == false || StackCount == 0)
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag or count was passed to AddOrRemoveStack"), ELogVerbosity::Warning);
		return;
	}

	(StackCount > 0) ? AddStack(Tag, StackCount) : RemoveStack(Tag, FMath::Abs(StackCount));
}

void FSFGameplayTagStackContainer::AddStack(FGameplayTag Tag, int32 StackCount)
{
	if (Tag.IsValid() == false || StackCount <= 0)
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag or count was passed to AddStack"), ELogVerbosity::Warning);
		return;
	}
	
	for (FSFGameplayTagStack& Stack : Stacks)
	{
		if (Stack.Tag == Tag)
		{
			const int32 NewCount = Stack.StackCount + StackCount;
			Stack.StackCount = NewCount;
			TagToCountMap[Tag] = NewCount;
			MarkItemDirty(Stack);
			return;
		}
	}

	FSFGameplayTagStack& NewStack = Stacks.Emplace_GetRef(Tag, StackCount);
	MarkItemDirty(NewStack);
	TagToCountMap.Add(Tag, StackCount);
	TagContainer.AddTag(Tag);
}

void FSFGameplayTagStackContainer::RemoveStack(FGameplayTag Tag, int32 StackCount)
{
	if (Tag.IsValid() == false || StackCount <= 0)
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag or count was passed to RemoveStack"), ELogVerbosity::Warning);
		return;
	}

	for (auto It = Stacks.CreateIterator(); It; ++It)
	{
		FSFGameplayTagStack& Stack = *It;
		if (Stack.Tag == Tag)
		{
			if (Stack.StackCount <= StackCount)
			{
				It.RemoveCurrent();
				TagToCountMap.Remove(Tag);
				TagContainer.RemoveTag(Tag);
				MarkArrayDirty();
			}
			else
			{
				const int32 NewCount = Stack.StackCount - StackCount;
				Stack.StackCount = NewCount;
				TagToCountMap[Tag] = NewCount;
				MarkItemDirty(Stack);
			}
			return;
		}
	}
}

void FSFGameplayTagStackContainer::RemoveStack(FGameplayTag Tag)
{
	if (Tag.IsValid() == false)
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to RemoveStack"), ELogVerbosity::Warning);
		return;
	}
	
	for (auto It = Stacks.CreateIterator(); It; ++It)
	{
		FSFGameplayTagStack& Stack = *It;
		if (Stack.Tag == Tag)
		{
			It.RemoveCurrent();
			TagToCountMap.Remove(Tag);
			TagContainer.RemoveTag(Tag);
			MarkArrayDirty();
			return;
		}
	}
}

int32 FSFGameplayTagStackContainer::GetStackCount(FGameplayTag Tag) const
{
	return TagToCountMap.FindRef(Tag);
}

bool FSFGameplayTagStackContainer::ContainsTag(FGameplayTag Tag) const
{
	return TagToCountMap.Contains(Tag);
}

void FSFGameplayTagStackContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		const FGameplayTag Tag = Stacks[Index].Tag;
		TagToCountMap.Remove(Tag);
		TagContainer.RemoveTag(Tag);
	}
}

void FSFGameplayTagStackContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FSFGameplayTagStack& Stack = Stacks[Index];
		TagToCountMap.Add(Stack.Tag, Stack.StackCount);
		TagContainer.AddTag(Stack.Tag);
	}
}

void FSFGameplayTagStackContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		const FSFGameplayTagStack& Stack = Stacks[Index];
		TagToCountMap[Stack.Tag] = Stack.StackCount;
	}
}
