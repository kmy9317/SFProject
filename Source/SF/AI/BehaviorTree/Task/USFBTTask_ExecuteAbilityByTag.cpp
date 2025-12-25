// Fill out your copyright notice in the Description page of Project Settings.

#include "USFBTTask_ExecuteAbilityByTag.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/SFCharacterGameplayTags.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"

UUSFBTTask_ExecuteAbilityByTag::UUSFBTTask_ExecuteAbilityByTag( const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    NodeName = "Execute Ability By Tag";
    bNotifyTick = false;
    bNotifyTaskFinished = true;
    bCreateNodeInstance = true;
}

UAbilitySystemComponent* UUSFBTTask_ExecuteAbilityByTag::GetASC(UBehaviorTreeComponent& OwnerComp) const
{
    if (AAIController* AIController = OwnerComp.GetAIOwner())
    {
        return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent( AIController->GetPawn());
    }
    return nullptr;
}

EBTNodeResult::Type UUSFBTTask_ExecuteAbilityByTag::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    CachedOwnerComp = &OwnerComp;
    
    UAbilitySystemComponent* ASC = GetASC(OwnerComp);
    if (!ASC)
    {
        return EBTNodeResult::Failed;
    }

    const FName AbilityTagName = OwnerComp.GetBlackboardComponent()->GetValueAsName(
        AbilityTagKey.SelectedKeyName);
    
    if (!AbilityTagName.IsValid())
    {
        return EBTNodeResult::Failed;
    }

    FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName);
    if (!AbilityTag.IsValid())
    {
        return EBTNodeResult::Failed;
    }

    // 실행 가능한 어빌리티 찾기
    TArray<FGameplayAbilitySpec*> Specs;
    ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
        FGameplayTagContainer(AbilityTag), Specs, true);

    if (Specs.Num() == 0)
    {

        return EBTNodeResult::Failed;
    }

    // 사용 가능한 첫 번째 어빌리티 활성화 시도
    bool bActivated = false;
    for (FGameplayAbilitySpec* Spec : Specs)
    {
        if (!Spec) continue;
        
        if (ASC->TryActivateAbility(Spec->Handle))
        {
            ExecutingAbilityHandle = Spec->Handle;
            bActivated = true;
            break;
        }
    }

    if (!bActivated)
    {
        return EBTNodeResult::Failed;
    }
    
    AbilityEndedHandle = ASC->OnAbilityEnded.AddUObject(this, &UUSFBTTask_ExecuteAbilityByTag::OnAbilityEnded);

    return EBTNodeResult::InProgress;
}

void UUSFBTTask_ExecuteAbilityByTag::OnAbilityEnded(const FAbilityEndedData& EndedData)
{
   
    if (EndedData.AbilitySpecHandle != ExecutingAbilityHandle)
        return;

    if (!CachedOwnerComp.IsValid())
        return;

    UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
    
    CleanupDelegates(*OwnerComp);
    
    // 어빌리티가 취소되었는지 정상 완료되었는지 체크
    if (EndedData.bWasCancelled)
    {
        FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
    }
    else
    {
        FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
    }
    
}

EBTNodeResult::Type UUSFBTTask_ExecuteAbilityByTag::AbortTask(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UAbilitySystemComponent* ASC = GetASC(OwnerComp);
    if (ASC && ExecutingAbilityHandle.IsValid())
    {
        ASC->CancelAbilityHandle(ExecutingAbilityHandle);
    }
    
    CleanupDelegates(OwnerComp);
    
    return EBTNodeResult::Aborted;
}

void UUSFBTTask_ExecuteAbilityByTag::OnTaskFinished(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, 
    EBTNodeResult::Type TaskResult)
{
    CleanupDelegates(OwnerComp);
    CachedOwnerComp.Reset();
    ExecutingAbilityHandle = FGameplayAbilitySpecHandle();
    
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UUSFBTTask_ExecuteAbilityByTag::CleanupDelegates(UBehaviorTreeComponent& OwnerComp)
{
    if (UAbilitySystemComponent* ASC = GetASC(OwnerComp))
    {
        if (AbilityEndedHandle.IsValid())
        {
            ASC->OnAbilityEnded.Remove(AbilityEndedHandle);
            AbilityEndedHandle.Reset();
        }
    }
}