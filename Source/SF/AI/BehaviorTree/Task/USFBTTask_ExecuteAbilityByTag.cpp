#include "USFBTTask_ExecuteAbilityByTag.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UUSFBTTask_ExecuteAbilityByTag::UUSFBTTask_ExecuteAbilityByTag(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    NodeName = "Execute Ability By Tag";
    bNotifyTick = false;
    bNotifyTaskFinished = true;
}

UAbilitySystemComponent* UUSFBTTask_ExecuteAbilityByTag::GetASC(UBehaviorTreeComponent& OwnerComp) const
{
    if (AAIController* AIController = OwnerComp.GetAIOwner())
    {
        if (APawn* Pawn = AIController->GetPawn())
        {
            return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
        }
    }
    return nullptr;
}

EBTNodeResult::Type UUSFBTTask_ExecuteAbilityByTag::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    CachedOwnerComp = &OwnerComp;
    bFinished = false;

    UAbilitySystemComponent* ASC = GetASC(OwnerComp);
    if (!ASC)
        return EBTNodeResult::Failed;

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB)
        return EBTNodeResult::Failed;

    FName TagName = BB->GetValueAsName(AbilityTagKey.SelectedKeyName);
    if (TagName == NAME_None)
        return EBTNodeResult::Failed;

    WatchedTag = FGameplayTag::RequestGameplayTag(TagName);
    if (!WatchedTag.IsValid())
        return EBTNodeResult::Failed;

    // 능력 찾기
    TArray<FGameplayAbilitySpec*> MatchingAbilities;
    FGameplayTagContainer SearchTags;
    SearchTags.AddTag(WatchedTag);

    ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(SearchTags, MatchingAbilities, true);

    if (MatchingAbilities.Num() == 0)
        return EBTNodeResult::Failed;

    // Ability 실행 시도
    bool bActivated = false;
    for (FGameplayAbilitySpec* Spec : MatchingAbilities)
    {
        if (Spec && Spec->Ability)
        {
            if (ASC->TryActivateAbility(Spec->Handle))
            {
                bActivated = true;
                break;
            }
        }
    }

    if (!bActivated)
        return EBTNodeResult::Failed;

    // Delegate 등록 (활성화 성공 후에만)
    if (!EventHandle.IsValid())
    {
        EventHandle = ASC->RegisterGameplayTagEvent(WatchedTag, EGameplayTagEventType::NewOrRemoved)
            .AddUObject(this, &UUSFBTTask_ExecuteAbilityByTag::OnTagChanged);
    }

    return EBTNodeResult::InProgress;
}

void UUSFBTTask_ExecuteAbilityByTag::OnTagChanged(FGameplayTag Tag, int32 NewCount)
{
    // 중복 호출 방지
    if (bFinished)
        return;

    // Ability 종료 조건 (태그 카운트가 0이 됨)
    if (NewCount != 0)
        return;

    bFinished = true;

    if (UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get())
    {
        CleanupDelegate(*OwnerComp);

        // Blackboard 초기화는 태스크 완료 직전에
        if (UBlackboardComponent* BB = OwnerComp->GetBlackboardComponent())
        {
            BB->ClearValue(AbilityTagKey.SelectedKeyName);
        }

        FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
    }
}

void UUSFBTTask_ExecuteAbilityByTag::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    CleanupDelegate(OwnerComp);
    
    CachedOwnerComp.Reset();
    WatchedTag = FGameplayTag();
    bFinished = false;

    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UUSFBTTask_ExecuteAbilityByTag::CleanupDelegate(UBehaviorTreeComponent& OwnerComp)
{
    if (!EventHandle.IsValid())
        return;

    if (UAbilitySystemComponent* ASC = GetASC(OwnerComp))
    {
        ASC->UnregisterGameplayTagEvent(EventHandle, WatchedTag, EGameplayTagEventType::NewOrRemoved);
    }
    
    EventHandle.Reset();
}