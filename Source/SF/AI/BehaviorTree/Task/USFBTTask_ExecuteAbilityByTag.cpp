// Fill out your copyright notice in the Description page of Project Settings.


#include "USFBTTask_ExecuteAbilityByTag.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"


UUSFBTTask_ExecuteAbilityByTag::UUSFBTTask_ExecuteAbilityByTag(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    NodeName = "Ability by Tag";
    bNotifyTick = false;
    bNotifyTaskFinished = true;  
}

UAbilitySystemComponent* UUSFBTTask_ExecuteAbilityByTag::GetASC(UBehaviorTreeComponent& OwnerComp) const  
{
    
    AAIController* AIController = OwnerComp.GetAIOwner(); 
    if (!AIController)
        return nullptr;
    
    APawn* ControlledPawn = AIController->GetPawn();
    if (!ControlledPawn)
        return nullptr;
    
    return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledPawn);
}

EBTNodeResult::Type UUSFBTTask_ExecuteAbilityByTag::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    CachedOwnerComp = &OwnerComp;
    UAbilitySystemComponent* ASC = GetASC(OwnerComp);
    if (!ASC)
    {
        return EBTNodeResult::Failed;
    }


    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp || AbilityTagKey.SelectedKeyName == NAME_None)
    {
        return EBTNodeResult::Failed;
    }

    FName TagName = BlackboardComp->GetValueAsName(AbilityTagKey.SelectedKeyName);
    if (TagName == NAME_None)
    {
        return EBTNodeResult::Failed;
    }

    //  FName → FGameplayTag 변환
    WatchedTag = FGameplayTag::RequestGameplayTag(TagName);
    if (!WatchedTag.IsValid())
    {
       
        return EBTNodeResult::Failed;
    }
    

    //  Ability 찾기
    TArray<FGameplayAbilitySpec*> MatchingAbilities;
    FGameplayTagContainer SearchTags;
    SearchTags.AddTag(WatchedTag);

    ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(SearchTags, MatchingAbilities, true);

    if (MatchingAbilities.Num() == 0)
    {
        return EBTNodeResult::Failed;
    }

    //  Ability 활성화 시도
    bool bActivated = false;
    for (FGameplayAbilitySpec* Spec : MatchingAbilities)
    {
        if (!Spec || !Spec->Ability)
        {
            continue;
        }

        //  이벤트 핸들러 등록 
        EventHandle = ASC->RegisterGameplayTagEvent(
            WatchedTag,
            EGameplayTagEventType::NewOrRemoved
        ).AddUObject(this, &UUSFBTTask_ExecuteAbilityByTag::ReceiveTagChanged);

        //  Ability 활성화
        bool bSuccess = ASC->TryActivateAbility(Spec->Handle);

        if (bSuccess)
        {
            bActivated = true;
            break;
        }
        else
        {
            // 활성화 실패 시 핸들러 제거
            if (EventHandle.IsValid())
            {
                ASC->RegisterGameplayTagEvent(
                    WatchedTag,
                    EGameplayTagEventType::NewOrRemoved
                ).Remove(EventHandle);
                EventHandle.Reset();
            }
        }
    }

    if (!bActivated)
    {
        return EBTNodeResult::Failed;
    }

    return EBTNodeResult::InProgress;
}

void UUSFBTTask_ExecuteAbilityByTag::ReceiveTagChanged(FGameplayTag Tag, int32 NewCount)
{
    //  Tag가 제거될 때 (Ability 종료)
    if (NewCount == 0)
    {
        UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
        if (!OwnerComp)
        {
            return;
        }

        //  Delegate 정리
        UAbilitySystemComponent* ASC = GetASC(*OwnerComp);
        if (ASC && EventHandle.IsValid())
        {
            ASC->RegisterGameplayTagEvent(
                WatchedTag,
                EGameplayTagEventType::NewOrRemoved
            ).Remove(EventHandle);
            EventHandle.Reset();
        }

    
        FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
    }
}

void UUSFBTTask_ExecuteAbilityByTag::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    UAbilitySystemComponent* ASC = GetASC(OwnerComp);
    if (ASC && EventHandle.IsValid())
    {
        ASC->RegisterGameplayTagEvent(
            WatchedTag,
            EGameplayTagEventType::NewOrRemoved
        ).Remove(EventHandle);
        EventHandle.Reset();
    }
    
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (BlackboardComp && AbilityTagKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->ClearValue(AbilityTagKey.SelectedKeyName);
    }
    
    CachedOwnerComp.Reset();
    WatchedTag = FGameplayTag();  

    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}