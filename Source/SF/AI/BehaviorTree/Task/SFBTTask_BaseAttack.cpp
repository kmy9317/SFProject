#include "SFBTTask_BaseAttack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AIController.h"
#include "Character/SFCharacterGameplayTags.h"

USFBTTask_BaseAttack::USFBTTask_BaseAttack(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    NodeName = "Attack by Tag";
    bNotifyTick = false;
    bNotifyTaskFinished = true;  
}

UAbilitySystemComponent* USFBTTask_BaseAttack::GetASC(UBehaviorTreeComponent& OwnerComp) const  
{
    AAIController* AIController = OwnerComp.GetAIOwner(); 
    if (!AIController)
        return nullptr;
    
    APawn* ControlledPawn = AIController->GetPawn();
    if (!ControlledPawn)
        return nullptr;
    
    return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledPawn);
}

EBTNodeResult::Type USFBTTask_BaseAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    CachedOwnerComp = &OwnerComp;  
    
    UAbilitySystemComponent* ASC = GetASC(OwnerComp);  
    if (!ASC)
    {
        return EBTNodeResult::Failed;
    }
    
    if (!AttackTag.IsValid())
    {
        return EBTNodeResult::Failed;
    }
    
    // 태그가  제거될 때 호출
    EventHandle = ASC->RegisterGameplayTagEvent(
        SFGameplayTags::Character_State_Attacking, 
        EGameplayTagEventType::NewOrRemoved
    ).AddUObject(this, &USFBTTask_BaseAttack::OnAttackingTagChanged);
    
    // Ability 활성화
    FGameplayTagContainer AttackTags;
    AttackTags.AddTag(AttackTag);
    
    bool bSuccess = ASC->TryActivateAbilitiesByTag(AttackTags);
    if (!bSuccess)
    {
        // 실패 시 Delegate 정리
        ASC->RegisterGameplayTagEvent(
            SFGameplayTags::Character_State_Attacking, 
            EGameplayTagEventType::NewOrRemoved
        ).Remove(EventHandle);
        EventHandle.Reset();
        return EBTNodeResult::Failed;
    }
    
    return EBTNodeResult::InProgress;
}

void USFBTTask_BaseAttack::OnAttackingTagChanged(const FGameplayTag Tag, int32 NewCount)
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
                SFGameplayTags::Character_State_Attacking, 
                EGameplayTagEventType::NewOrRemoved
            ).Remove(EventHandle);
            EventHandle.Reset();
        }
        
        FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
    }
}

void USFBTTask_BaseAttack::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    // Task가 어떤 이유로든 종료될 때 Delegate 정리
    UAbilitySystemComponent* ASC = GetASC(OwnerComp);
    if (ASC && EventHandle.IsValid())
    {
        ASC->RegisterGameplayTagEvent(
            SFGameplayTags::Character_State_Attacking, 
            EGameplayTagEventType::NewOrRemoved
        ).Remove(EventHandle);
        EventHandle.Reset();
    }
    
    CachedOwnerComp.Reset(); 
    
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}