#include "SFGA_Hero_Consume.h"

#include "AbilitySystemComponent.h"
#include "Item/SFItemInstance.h"
#include "Item/Fragments/SFItemFragment_Consumable.h"

USFGA_Hero_Consume::USFGA_Hero_Consume(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void USFGA_Hero_Consume::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    bItemUsed = false;
    ConsumeItemInstance = nullptr;
    ConsumeFragment = nullptr;
    ConsumeSlotHandle = FSFItemSlotHandle();

    if (TriggerEventData)
    {
        // ItemInstance 추출
        if (TriggerEventData->OptionalObject)
        {
            ConsumeItemInstance = Cast<USFItemInstance>(const_cast<UObject*>(TriggerEventData->OptionalObject.Get()));
            if (ConsumeItemInstance)
            {
                ConsumeFragment = ConsumeItemInstance->FindFragmentByClass<USFItemFragment_Consumable>();
            }
        }

        // SlotIndex 추출
        int32 SlotIndex = FMath::RoundToInt(TriggerEventData->EventMagnitude);
        ConsumeSlotHandle = FSFItemSlotHandle(ESFItemSlotType::Quickbar, SlotIndex);
    }

    if (!ConsumeItemInstance || !ConsumeFragment)
    {
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
    }
}

int32 USFGA_Hero_Consume::GetConsumeCount() const
{
    if (ConsumeFragment)
    {
        return ConsumeFragment->ConsumeCount;
    }
    return 1;
}

void USFGA_Hero_Consume::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    // 아이템 사용 성공 시 Cost 적용
    if (bItemUsed && !bWasCancelled)
    {
        ApplyCost(Handle, ActorInfo, ActivationInfo);
    }

    ConsumeItemInstance = nullptr;
    ConsumeFragment = nullptr;
    ConsumeSlotHandle = FSFItemSlotHandle();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGA_Hero_Consume::ApplyConsumeEffect()
{
    if (!ConsumeFragment || !ConsumeItemInstance)
    {
        return;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC)
    {
        return;
    }

    TSubclassOf<UGameplayEffect> EffectClass = ConsumeFragment->ConsumeEffect;
    if (!EffectClass)
    {
        return;
    }

    FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
    Context.AddSourceObject(ConsumeItemInstance);

    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1, Context);
    if (!SpecHandle.IsValid())
    {
        return;
    }

    // 지정된 Attribute들에 대한 SetByCaller 적용
    const FGameplayTag& RarityTag = ConsumeItemInstance->GetItemRarityTag();
    ConsumeFragment->ApplySetByCallersToSpec(SpecHandle.Data.Get(), RarityTag);

    ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}
