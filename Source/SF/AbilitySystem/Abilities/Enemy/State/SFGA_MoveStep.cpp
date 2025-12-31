#include "SFGA_MoveStep.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/SFAIGameplayTags.h"
#include "Character/SFCharacterGameplayTags.h"

class AAIController;

USFGA_MoveStep::USFGA_MoveStep()
{
    
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    ActivationOwnedTags.AddTag(SFGameplayTags::Ability_Enemy_Movement_Step); 
    ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_UsingAbility);
    ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Launched);

    
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        FAbilityTriggerData StepTrigger;
        StepTrigger.TriggerTag = FGameplayTag::RequestGameplayTag("GameplayEvent.MoveStep");
        StepTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
        AbilityTriggers.Add(StepTrigger);
    }
}

void USFGA_MoveStep::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayAbilityActivationInfo ActivationInfo, 
    const FGameplayEventData* TriggerEventData)
{
    if (IsActive())
    {
        return;
    }
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character) return;


    if (AAIController* AIC = Cast<AAIController>(Character->GetController()))
    {
        AIC->StopMovement();
    }
    
    float MoveDirection = TriggerEventData ? TriggerEventData->EventMagnitude : 1.0f;

    if (FMath::IsNearlyZero(MoveDirection)) MoveDirection = 1.0f;

    FVector ForwardV = Character->GetActorForwardVector();
    FVector LaunchVelocity = ForwardV * MoveDirection * StepIntensity;
    LaunchVelocity.Z = StepIntensity * UpForce; 


    Character->LaunchCharacter(LaunchVelocity, true, true);
    
 
    UAnimMontage* SelectedAnim = (MoveDirection > 0) ? ForwardAnim : BackwardAnim;
    if (SelectedAnim)
    {
        UAbilityTask_PlayMontageAndWait* MontageTask = 
            UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SelectedAnim);

        if (MontageTask)
        {
            MontageTask->OnCompleted.AddDynamic(this, &USFGA_MoveStep::OnMoveStepFinished);
            MontageTask->OnInterrupted.AddDynamic(this, &USFGA_MoveStep::OnMoveStepFinished);
            MontageTask->OnCancelled.AddDynamic(this, &USFGA_MoveStep::OnMoveStepFinished);
            MontageTask->ReadyForActivation();
        }
    }
    else
    {
        OnMoveStepFinished();
    }
}

void USFGA_MoveStep::OnMoveStepFinished()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}
