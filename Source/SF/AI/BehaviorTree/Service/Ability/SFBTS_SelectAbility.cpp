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

    // ==============================================================================
    // [수정] 무한 정지 방지 로직
    // AI가 현재 '공격 중(Attacking)'이라면, 스킬 선택 로직을 돌리지 않고 리턴합니다.
    // 이유: 공격 중에 SelectAbility가 실패(쿨타임 등)해서 키를 지워버리면,
    //       데코레이터가 이를 감지하고 공격을 강제 취소(Abort)시켜버리기 때문입니다.
    // ==============================================================================
    if (APawn* Pawn = AIController->GetPawn())
    {
        UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
        // Character.State.Attacking 태그가 있다면(공격 중이라면) 블랙보드 유지
        if (ASC && ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Attacking))
        {
            return;
        }
    }
    // ==============================================================================

    // ==============================================================================
    // [추가] 이동 중 블랙보드 유지 로직
    // AI가 현재 '이동 중(Moving)'이라면, 스킬 선택 로직을 돌리지 않고 리턴합니다.
    // 이유: 이동 중에 SelectAbility가 실패(사거리 밖)해서 키를 지워버리면,
    //       데코레이터가 이를 감지하고 이동을 강제 취소(Abort)시켜버리기 때문입니다.
    //       이렇게 되면 AI가 영원히 목적지에 도달하지 못하고 제자리에서 멈춥니다.
    // ==============================================================================
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

    FGameplayTag SelectedTag;

    // 스킬 선택 시도
    if (CombatComp->SelectAbility(Context, AbilitySearchTags, SelectedTag))
    {
        // 성공: 사용 가능한 스킬이 있으므로 블랙보드 갱신
        BB->SetValueAsName(BlackboardKey.SelectedKeyName, SelectedTag.GetTagName());
    }
    else
    {
        // 실패: 쓸 수 있는 스킬이 없음 (쿨타임, 사거리 밖 등) -> 키 삭제
        // (위의 방어 로직 덕분에 공격 중에는 이 부분이 실행되지 않음)
        BB->ClearValue(BlackboardKey.SelectedKeyName);
    }
}