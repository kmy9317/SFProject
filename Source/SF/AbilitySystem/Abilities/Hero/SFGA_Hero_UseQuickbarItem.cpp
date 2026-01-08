#include "SFGA_Hero_UseQuickbarItem.h"

#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Inventory/SFQuickbarComponent.h"
#include "Item/SFItemManagerComponent.h"
#include "Item/SFItemGameplayTags.h"
#include "Item/SFItemInstance.h"
#include "Item/Fragments/SFItemFragment_Consumable.h"

USFGA_Hero_UseQuickbarItem::USFGA_Hero_UseQuickbarItem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = ESFAbilityActivationPolicy::Manual;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;

	// 입력 단계 체크용 태그 (확장 시 사용)
	// ActivationBlockedTags.AddTag(SFGameplayTags::Status_Stunned);
	// ActivationBlockedTags.AddTag(SFGameplayTags::Status_Casting);

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = SFGameplayTags::GameplayEvent_UseQuickbar;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void USFGA_Hero_UseQuickbarItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	int32 SlotIndex = FMath::RoundToInt(TriggerEventData->EventMagnitude);

	APlayerController* PC = Cast<APlayerController>(ActorInfo->PlayerController.Get());
	if (!PC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	USFQuickbarComponent* QuickbarComponent = PC->FindComponentByClass<USFQuickbarComponent>();
	if (!QuickbarComponent || QuickbarComponent->IsSlotEmpty(SlotIndex))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Fragment에서 소모 갯수 체크 
	USFItemInstance* ItemInstance = QuickbarComponent->GetItemInstance(SlotIndex);
	int32 CurrentCount = QuickbarComponent->GetItemCount(SlotIndex);
	int32 RequiredCount = 1;

	if (ItemInstance)
	{
		if (const USFItemFragment_Consumable* ConsumeFrag = ItemInstance->FindFragmentByClass<USFItemFragment_Consumable>())
		{
			RequiredCount = ConsumeFrag->ConsumeCount;
		}
	}

	if (CurrentCount < RequiredCount)
	{
		// 수량 부족 → RPC 안 보냄
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (USFItemManagerComponent* ItemManager = PC->FindComponentByClass<USFItemManagerComponent>())
	{
		FSFItemSlotHandle Slot(ESFItemSlotType::Quickbar, SlotIndex);
		ItemManager->Server_UseItem(Slot);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}