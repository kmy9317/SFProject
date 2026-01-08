#include "SFAbilityCost_QuickbarItem.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "AbilitySystem/Abilities/Hero/Consumable/SFGA_Hero_Consume.h"
#include "Inventory/SFQuickbarComponent.h"
#include "Item/SFItemManagerComponent.h"

USFAbilityCost_QuickbarItem::USFAbilityCost_QuickbarItem()
{
    
}

bool USFAbilityCost_QuickbarItem::CheckCost(const USFGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
    return true;
}

void USFAbilityCost_QuickbarItem::ApplyCost(const USFGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
    if (!ActorInfo->IsNetAuthority())
    {
        return;
    }

    const USFGA_Hero_Consume* ConsumeAbility = Cast<const USFGA_Hero_Consume>(Ability);
    if (!ConsumeAbility)
    {
        return;
    }

    const FSFItemSlotHandle& SlotHandle = ConsumeAbility->GetConsumeSlotHandle();
    if (!SlotHandle.IsValid())
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(ActorInfo->PlayerController.Get());
    if (!PC)
    {
        return;
    }

    USFItemManagerComponent* ItemManager = PC->FindComponentByClass<USFItemManagerComponent>();
    if (ItemManager)
    {
        int32 ConsumeCount = ConsumeAbility->GetConsumeCount();
        ItemManager->ConsumeFromSlot(SlotHandle, ConsumeCount);
    }
}