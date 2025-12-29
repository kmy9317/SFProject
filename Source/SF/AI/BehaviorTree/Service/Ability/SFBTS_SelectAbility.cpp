// AI/BehaviorTree/Service/Ability/SFBTS_SelectAbility.cpp

#include "SFBTS_SelectAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h" // [필수 추가] ASC 가져오기 위해 필요
#include "AIController.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Interface/SFEnemyAbilityInterface.h"
#include "Character/SFCharacterGameplayTags.h" // [필수 추가] Attacking 태그 확인용
#include "Navigation/PathFollowingComponent.h" // [추가] 이동 상태 체크용

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

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return;

    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return;
    }
    
    if (APawn* Pawn = AIController->GetPawn())
    {
        UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
        // Character.State.Attacking 태그가 있다면(공격 중이라면) 블랙보드 유지
        if (ASC && ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Attacking))
        {
            return;
        }
    }
    if (AIController->GetMoveStatus() != EPathFollowingStatus::Idle)
    {
        return; // 이동 중에는 블랙보드 유지
    }
    // ==============================================================================

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
    
    if (!Context.Target || !IsValid(Context.Target))
    {
        BB->ClearValue(BlackboardKey.SelectedKeyName);
        return;
    }

    const FName CurrentAbilityTagName = BB->GetValueAsName(BlackboardKey.SelectedKeyName);
    if (!CurrentAbilityTagName.IsNone())
    {
        // 이미 선택된 Ability가 있으면 유지
        return;
    }

    FGameplayTag SelectedTag;
    
    if (CombatComp->SelectAbility(Context, AbilitySearchTags, SelectedTag))
    {
        // 성공: 사용 가능한 스킬이 있으므로 블랙보드 갱신
        BB->SetValueAsName(BlackboardKey.SelectedKeyName, SelectedTag.GetTagName());
    }

}