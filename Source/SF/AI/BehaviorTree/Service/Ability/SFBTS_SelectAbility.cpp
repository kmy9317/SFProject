// SFBTS_SelectAbility.cpp
#include "SFBTS_SelectAbility.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Interface/SFEnemyAbilityInterface.h"

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

    // 이미 선택된 Ability가 있으면 stop 
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (BB && BB->GetValueAsName(BlackboardKey.SelectedKeyName) != NAME_None)
    {
        return;
    }
    
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return;
    }
    
    USFEnemyCombatComponent* CombatComp = USFEnemyCombatComponent::FindSFEnemyCombatComponent(AIController);
    if (!CombatComp)
    {
        return;
    }

    FEnemyAbilitySelectContext Context;
    APawn* AIPawn = AIController->GetPawn();
    if (!AIPawn)
    {
        return;
    }
    
    Context.Self   = AIPawn;
    Context.Target = Cast<AActor>(BB->GetValueAsObject(TargetKey.SelectedKeyName));

    FGameplayTag SelectedTag;
    if (CombatComp->SelectAbility(Context, AbilitySearchTags, SelectedTag))
    {
        BB->SetValueAsName(BlackboardKey.SelectedKeyName, SelectedTag.GetTagName());
    }
}