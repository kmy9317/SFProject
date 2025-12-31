// Fill out your copyright notice in the Description page of Project Settings.

#include "SFBTT_MoveStep.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/Abilities/Enemy/State/SFGA_MoveStep.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/SFAIGameplayTags.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/SFCharacterGameplayTags.h"

USFBTT_MoveStep::USFBTT_MoveStep()
{
    NodeName = "Move Step Task";

    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(USFBTT_MoveStep, TargetKey), AActor::StaticClass());
}

EBTNodeResult::Type USFBTT_MoveStep::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AICon = OwnerComp.GetAIOwner();
    if (!AICon) return EBTNodeResult::Failed;

    APawn* Pawn = AICon->GetPawn();
    if (!Pawn) return EBTNodeResult::Failed;

    UAbilitySystemComponent* ASC = Pawn->FindComponentByClass<UAbilitySystemComponent>();
    if (!ASC || !AbilityClass) return EBTNodeResult::Failed;

    AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TargetKey.SelectedKeyName));
    if (!Target) return EBTNodeResult::Failed;

    
    AbilityEndedHandle = ASC->OnAbilityEnded.AddUObject(this, &USFBTT_MoveStep::OnAbilityEnded, TWeakObjectPtr<UBehaviorTreeComponent>(&OwnerComp));
    
    FVector Forward = Pawn->GetActorForwardVector();
    FVector ToTarget = (Target->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal();
    float Dot = FVector::DotProduct(Forward, ToTarget);

    float StepDirection = (Dot >= 0.f) ? 1.f : -1.f; 

 
    FGameplayEventData Payload;
    Payload.EventTag = SFGameplayTags::GameplayEvent_Launched;
    Payload.EventMagnitude = StepDirection;
    ASC->HandleGameplayEvent(Payload.EventTag, &Payload);


    RunningAbilityHandle = FGameplayAbilitySpecHandle();

    FGameplayTag MoveStepTag = SFGameplayTags::Ability_Enemy_Attack_Charge;
    TArray<FGameplayAbilitySpec*> ActiveSpecs;

    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        if (Spec.Ability && Spec.Ability->AbilityTags.HasTag(SFGameplayTags::Ability_Enemy_Movement_Step) && Spec.IsActive())
        {
            RunningAbilityHandle = Spec.Handle;
            break;
        }
    }

    if (!RunningAbilityHandle.IsValid())
    {
        Cleanup(&OwnerComp);
        return EBTNodeResult::Failed;
    }

    return EBTNodeResult::InProgress;
}

void USFBTT_MoveStep::OnAbilityEnded(const FAbilityEndedData& EndedData, TWeakObjectPtr<UBehaviorTreeComponent> OwnerCompPtr)
{
    if (EndedData.AbilitySpecHandle == RunningAbilityHandle)
    {
        if (UBehaviorTreeComponent* OwnerComp = OwnerCompPtr.Get())
        {
            Cleanup(OwnerComp);
            RunningAbilityHandle = FGameplayAbilitySpecHandle();
            FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
        }
    }
}

void USFBTT_MoveStep::Cleanup(UBehaviorTreeComponent* OwnerComp)
{
    if (UAbilitySystemComponent* ASC = OwnerComp->GetAIOwner()->GetPawn()->FindComponentByClass<UAbilitySystemComponent>())
    {
        ASC->OnAbilityEnded.Remove(AbilityEndedHandle);
    }
}
