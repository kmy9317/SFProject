// SFBTS_SelectAbility.cpp
#include "SFBTS_SelectAbility.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

USFBTS_SelectAbility::USFBTS_SelectAbility()
{
    NodeName = TEXT("Select Ability");
    Interval = 0.2f; 
}

uint16 USFBTS_SelectAbility::GetInstanceMemorySize() const
{
    return sizeof(FBTSelectAbilityMemory);
}

void USFBTS_SelectAbility::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
    Super::InitializeMemory(OwnerComp, NodeMemory, InitType);
    
    if (InitType == EBTMemoryInit::Initialize)
    {
        new (NodeMemory) FBTSelectAbilityMemory();
    }
}

void USFBTS_SelectAbility::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);


    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (BlackboardComp)
    {
        FName CurrentAbility = BlackboardComp->GetValueAsName(BlackboardKey.SelectedKeyName);
        if (CurrentAbility != NAME_None)
        {
            return; 
        }
    }

    APawn* AIPawn = OwnerComp.GetAIOwner()->GetPawn();
    if (!AIPawn)
    {
        return;
    }

    UAbilitySystemComponent* ASC = AIPawn->FindComponentByClass<UAbilitySystemComponent>();
    if (!ASC)
    {
        return;
    }
 
    FBTSelectAbilityMemory* MyMemory = reinterpret_cast<FBTSelectAbilityMemory*>(NodeMemory);

    FGameplayTag SelectedAbilityTag;

    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        if (!Spec.Ability)
        {
            continue;
        }

        bool bTagMatches = false;

        for (const FGameplayTag& SearchTag : AbilitySearchTags)
        {
            for (const FGameplayTag& AbilityTag : Spec.Ability->AbilityTags)
            {
                if (AbilityTag.MatchesTag(SearchTag))
                {
                    bTagMatches = true;
                    break;
                }
            }

            if (!bTagMatches && Spec.Ability->AbilityTags.Num() == 0)
            {
                for (const FGameplayTag& AssetTag : Spec.Ability->GetAssetTags())
                {
                    if (AssetTag.MatchesTag(SearchTag))
                    {
                        bTagMatches = true;
                        break;
                    }
                }
            }

            if (bTagMatches) break;
        }

        if (!bTagMatches)
        {
            continue;
        }
        
        bool bCanActivate = Spec.Ability->CanActivateAbility(Spec.Handle, ASC->AbilityActorInfo.Get());

        if (bCanActivate)
        {
            if (Spec.Ability->AbilityTags.Num() > 0)
            {
                SelectedAbilityTag = Spec.Ability->AbilityTags.First();
            }
            else if (Spec.Ability->GetAssetTags().Num() > 0)
            {
                SelectedAbilityTag = Spec.Ability->GetAssetTags().First();
            }
            break;
        }
    }
    
    if (SelectedAbilityTag != MyMemory->LastSelectedAbilityTag)
    {
        if (SelectedAbilityTag.IsValid())
        {
            OwnerComp.GetBlackboardComponent()->SetValueAsName(
                BlackboardKey.SelectedKeyName,
                SelectedAbilityTag.GetTagName()
            );
        }
        else
        {
            OwnerComp.GetBlackboardComponent()->ClearValue(BlackboardKey.SelectedKeyName);
        }
        MyMemory->LastSelectedAbilityTag = SelectedAbilityTag;
    }
}