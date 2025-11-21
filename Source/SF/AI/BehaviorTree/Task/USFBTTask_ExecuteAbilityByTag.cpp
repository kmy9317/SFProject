// Fill out your copyright notice in the Description page of Project Settings.


#include "USFBTTask_ExecuteAbilityByTag.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"


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
    
    if (!ExecuteTag.IsValid())
    {
        return EBTNodeResult::Failed;
    }
    
    TArray<FGameplayAbilitySpec*> MatchingAbilities;
    FGameplayTagContainer SearchTags;
    SearchTags.AddTag(ExecuteTag);
    
    ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(SearchTags, MatchingAbilities, true);
    
    if (MatchingAbilities.Num() == 0)
    {
        return EBTNodeResult::Failed;
    }
    
    // 모든 어빌리티를 순회하면서 활성화 시도
    bool bActivated = false;
    for (FGameplayAbilitySpec* Spec : MatchingAbilities)
    {
        if (!Spec || !Spec->Ability)
        {
            continue;
        }
        
        // 먼저 이벤트 핸들러 등록 (타이밍 이슈 방지)
        EventHandle = ASC->RegisterGameplayTagEvent(
            CallbackTag,
            EGameplayTagEventType::NewOrRemoved
        ).AddUObject(this, &UUSFBTTask_ExecuteAbilityByTag::ReceiveTagChanged);
        
        // 어빌리티 활성화 시도
        bool bSuccess = ASC->TryActivateAbility(Spec->Handle);
        
        if (bSuccess)
        {
            bActivated = true;
            break; 
        }
        else
        {
            // 활성화 실패 시 등록한 핸들러 제거
            if (EventHandle.IsValid())
            {
                ASC->RegisterGameplayTagEvent(
                    CallbackTag,
                    EGameplayTagEventType::NewOrRemoved
                ).Remove(EventHandle);
                EventHandle.Reset();
            }
        }
    }
    
    // 모든 어빌리티가 실패한 경우
    if (!bActivated)
    {
        return EBTNodeResult::Failed;
    }
    
    return EBTNodeResult::InProgress;
}

void UUSFBTTask_ExecuteAbilityByTag::ReceiveTagChanged(FGameplayTag Tag, int32 NewCount)
{
    // 태그가 제거될 때 
    if (NewCount == 0)
    {
        UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get(); 
        if (!OwnerComp)
        {
            return;
        }
        
        // Delegate 정리
        UAbilitySystemComponent* ASC = GetASC(*OwnerComp);  
        if (ASC && EventHandle.IsValid())
        {
            ASC->RegisterGameplayTagEvent(
                CallbackTag,
                EGameplayTagEventType::NewOrRemoved
            ).Remove(EventHandle);
            EventHandle.Reset();
        }
        
        FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
    }
}

void UUSFBTTask_ExecuteAbilityByTag::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    // Task가 어떤 이유로든 종료될 때 Delegate 정리
    UAbilitySystemComponent* ASC = GetASC(OwnerComp);
    if (ASC && EventHandle.IsValid())
    {
        ASC->RegisterGameplayTagEvent(
            CallbackTag,
            EGameplayTagEventType::NewOrRemoved
        ).Remove(EventHandle);
        EventHandle.Reset();
    }
    
    CachedOwnerComp.Reset(); 
    
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}