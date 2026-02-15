#include "SFGA_Hero_Grabbed.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Animation/SFAnimationGameplayTags.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/Hero/SFHero.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/SFPlayerController.h"

USFGA_Hero_Grabbed::USFGA_Hero_Grabbed(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    ActivationPolicy = ESFAbilityActivationPolicy::Manual;
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
    
    AbilityTags.AddTag(SFGameplayTags::Ability_Hero_Grabbed);
    ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Grabbed);
    ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Grabbed);

    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        FAbilityTriggerData TriggerData;
        TriggerData.TriggerTag = SFGameplayTags::GameplayEvent_Grabbed;
        TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
        AbilityTriggers.Add(TriggerData);
    }

    MontageTag = SFGameplayTags::Montage_State_Grabbed;
    ReleaseEventTag = SFGameplayTags::GameplayEvent_GrabRelease;
}

void USFGA_Hero_Grabbed::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (TriggerEventData)
    {
        GrabberActor = const_cast<AActor*>(TriggerEventData->Instigator.Get());
    }

    if (USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo())
    {
        ASC->CancelActiveAbilities(nullptr, nullptr, this);
    }

    DisablePlayerInput(true);
    PlayGrabbedMontage();

    // 해제 이벤트 대기
    UAbilityTask_WaitGameplayEvent* ReleaseWaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ReleaseEventTag, nullptr, false, true);
    if (ReleaseWaitTask)
    {
        ReleaseWaitTask->EventReceived.AddDynamic(this, &USFGA_Hero_Grabbed::OnReleaseEventReceived);
        ReleaseWaitTask->ReadyForActivation();
    }
}

void USFGA_Hero_Grabbed::PlayGrabbedMontage()
{
    const USFHeroAnimationData* AnimData = GetHeroAnimationData();
    if (!AnimData)
    {
        return;
    }

    FSFMontagePlayData MontageData = AnimData->GetSingleMontage(MontageTag);
    if (!MontageData.IsValid())
    {
        return;
    }

    UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("GrabbedMontage"),MontageData.Montage,MontageData.PlayRate,MontageData.StartSection,true,1.0f,0.0f,true);
    if (MontageTask)
    {
        MontageTask->OnInterrupted.AddDynamic(this, &USFGA_Hero_Grabbed::OnMontageInterrupted);
        MontageTask->OnCancelled.AddDynamic(this, &USFGA_Hero_Grabbed::OnMontageCancelled);
        MontageTask->ReadyForActivation();
    }
}

void USFGA_Hero_Grabbed::HandleGrabRelease()
{
    RestorePlayerInput(true); 
    
    ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
    if (Character)
    {
        if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
        {
            MovementComp->SetMovementMode(MOVE_Falling);
        }
    }
}

void USFGA_Hero_Grabbed::OnReleaseEventReceived(FGameplayEventData Payload)
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Hero_Grabbed::OnMontageInterrupted()
{
    // 몽타주 중단되어도 Release 이벤트까지 대기
}

void USFGA_Hero_Grabbed::OnMontageCancelled()
{

}

void USFGA_Hero_Grabbed::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    HandleGrabRelease();
    GrabberActor.Reset();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}