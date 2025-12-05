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

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return;

    // [삭제] 이 부분이 있으면 한 번 스킬을 정한 뒤 쿨타임이 돌아도 갱신되지 않는 문제가 발생합니다.
    /*
    if (BB && BB->GetValueAsName(BlackboardKey.SelectedKeyName) != NAME_None)
    {
        return;
    }
    */
    
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
    
    // [수정] 스킬 선택 시도
    if (CombatComp->SelectAbility(Context, AbilitySearchTags, SelectedTag))
    {
        // 성공: 사용 가능한 스킬이 있으므로 블랙보드 갱신
        BB->SetValueAsName(BlackboardKey.SelectedKeyName, SelectedTag.GetTagName());
    }
    else
    {
        // [추가] 중요: 쿨타임이거나 사거리 밖이라서 쓸 스킬이 없으면 태그를 지웁니다.
        // 태그가 지워져야 BT가 공격 노드를 탈출하고 이동/대기 로직으로 넘어갑니다.
        BB->ClearValue(BlackboardKey.SelectedKeyName);
    }
}