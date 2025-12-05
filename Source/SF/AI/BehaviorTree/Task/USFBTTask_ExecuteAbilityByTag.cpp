// Fill out your copyright notice in the Description page of Project Settings.

#include "USFBTTask_ExecuteAbilityByTag.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/SFCharacterGameplayTags.h" 

UUSFBTTask_ExecuteAbilityByTag::UUSFBTTask_ExecuteAbilityByTag(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    NodeName = "Execute Ability By Tag";
    
    // [중요] Tick을 켜야 멈춤 방지(타임아웃)가 작동합니다.
    bNotifyTick = true; 
    bNotifyTaskFinished = true;

    // [중요] 멤버 변수 사용을 위해 인스턴스화 필수
    bCreateNodeInstance = true;
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
    ElapsedTime = 0.0f; // 시간 초기화

    UAbilitySystemComponent* ASC = GetASC(OwnerComp);
    if (!ASC) return EBTNodeResult::Failed;

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return EBTNodeResult::Failed;

    FName TagName = BB->GetValueAsName(AbilityTagKey.SelectedKeyName);
    if (TagName == NAME_None) return EBTNodeResult::Failed;

    FGameplayTag AbilityTagToActivate = FGameplayTag::RequestGameplayTag(TagName);
    if (!AbilityTagToActivate.IsValid()) return EBTNodeResult::Failed;

    // 1. 어빌리티 실행
    TArray<FGameplayAbilitySpec*> MatchingAbilities;
    FGameplayTagContainer SearchTags;
    SearchTags.AddTag(AbilityTagToActivate);
    ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(SearchTags, MatchingAbilities, true);

    if (MatchingAbilities.Num() == 0) return EBTNodeResult::Failed;

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

    if (!bActivated) return EBTNodeResult::Failed;

    // 2. 대기할 태그 설정 (기본값: Character.State.Attacking)
    // 어빌리티 이름 태그가 아니라, 상태 태그를 기다려야 멈추지 않습니다.
    TagToWait = WaitForTag.IsValid() ? WaitForTag : SFGameplayTags::Character_State_Attacking;

    // 3. 태그 감시 등록
    if (ASC->GetTagCount(TagToWait) > 0)
    {
        if (!EventHandle.IsValid())
        {
            EventHandle = ASC->RegisterGameplayTagEvent(TagToWait, EGameplayTagEventType::NewOrRemoved)
                .AddUObject(this, &UUSFBTTask_ExecuteAbilityByTag::OnTagChanged);
        }
        return EBTNodeResult::InProgress;
    }
    else
    {
        // 태그가 즉시 사라졌거나 안 붙었다면 바로 성공 처리 (멈춤 방지)
        return EBTNodeResult::Succeeded;
    }
}

// [추가] 타임아웃 체크: 5초가 지나도 안 끝나면 강제 종료
void UUSFBTTask_ExecuteAbilityByTag::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

    if (bFinished) return;

    ElapsedTime += DeltaSeconds;

    if (MaxDuration > 0.0f && ElapsedTime >= MaxDuration)
    {
        bFinished = true;
        CleanupDelegate(OwnerComp);
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}

void UUSFBTTask_ExecuteAbilityByTag::OnTagChanged(const FGameplayTag Tag, int32 NewCount)
{
    if (bFinished) return;

    // 태그가 사라지면(0) 정상 종료
    if (NewCount == 0)
    {
        bFinished = true;
        if (UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get())
        {
            CleanupDelegate(*OwnerComp);
            FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
        }
    }
}

void UUSFBTTask_ExecuteAbilityByTag::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    CleanupDelegate(OwnerComp);
    
    CachedOwnerComp.Reset();
    TagToWait = FGameplayTag(); 
    bFinished = false;

    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UUSFBTTask_ExecuteAbilityByTag::CleanupDelegate(UBehaviorTreeComponent& OwnerComp)
{
    if (EventHandle.IsValid())
    {
        if (UAbilitySystemComponent* ASC = GetASC(OwnerComp))
        {
            ASC->UnregisterGameplayTagEvent(EventHandle, TagToWait, EGameplayTagEventType::NewOrRemoved);
        }
    }
    EventHandle.Reset();
}