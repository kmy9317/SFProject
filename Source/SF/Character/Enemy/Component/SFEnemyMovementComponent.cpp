// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/Component/SFEnemyMovementComponent.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/SFPrimarySet.h"
#include "AbilitySystem/Attributes/Enemy/SFPrimarySet_Enemy.h"
#include "AI/SFAIGameplayTags.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/SFPawnExtensionComponent.h"
#include "GameFramework/Character.h"


void USFEnemyMovementComponent::InitializeMovementComponent()
{
    MappingStateFunction();

    bUseRVOAvoidance = false;
    
    UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
    if (!ASC) return;
    
    ASC->GetGameplayAttributeValueChangeDelegate(USFPrimarySet_Enemy::GetMoveSpeedAttribute())
              .AddUObject(this, &ThisClass::OnMoveSpeedChanged);
    
    RegisterStateTagEvents(ASC);
}

void USFEnemyMovementComponent::OnMoveSpeedChanged(const FOnAttributeChangeData& OnAttributeChangeData)
{
    MaxWalkSpeed = OnAttributeChangeData.NewValue;
}

void USFEnemyMovementComponent::MappingStateFunction()
{
    // Start Functions
    StateStartMap.Add(SFGameplayTags::Character_State_Parried, [this]() { StartStateParried(); });
    StateStartMap.Add(SFGameplayTags::Character_State_Stunned, [this]() { StartStateStunned(); });
    StateStartMap.Add(SFGameplayTags::Character_State_Groggy, [this]() { StartStateGroggy(); });
    StateStartMap.Add(SFGameplayTags::Character_State_Dead, [this]() { StartStateDead(); });
    
  
    StateStartMap.Add(SFGameplayTags::Character_State_Hit, [this]() { StartStateHitReact(); });
    StateStartMap.Add(SFGameplayTags::Character_State_Knockback, [this]() { StartStateKnockback(); });
    StateStartMap.Add(SFGameplayTags::Character_State_Knockdown, [this]() { StartStateKnockdown(); });


    // End Functions
    StateEndMap.Add(SFGameplayTags::Character_State_Parried, [this]() { EndStateParried(); });
    StateEndMap.Add(SFGameplayTags::Character_State_Stunned, [this]() { EndStateStunned(); });
    StateEndMap.Add(SFGameplayTags::Character_State_Groggy, [this]() { EndStateGroggy(); });
    StateEndMap.Add(SFGameplayTags::Character_State_Dead, [this]() { EndStateDead(); });

 
    StateEndMap.Add(SFGameplayTags::Character_State_Hit, [this]() { EndStateHitReact(); });
    StateEndMap.Add(SFGameplayTags::Character_State_Knockback, [this]() { EndStateKnockback(); });
    StateEndMap.Add(SFGameplayTags::Character_State_Knockdown, [this]() { EndStateKnockdown(); });
}

void USFEnemyMovementComponent::InternalDisableMovement()
{
 
    StopMovementImmediately();
    
    if (MovementMode != MOVE_None)
    {
        PreviousMovementMode = MovementMode;
    }

    DisableMovement(); 
}


void USFEnemyMovementComponent::InternalEnableMovement()
{
  
    if (PreviousMovementMode == MOVE_None)
    {
        SetMovementMode(DefaultLandMovementMode);
    }
    else
    {
        SetMovementMode(PreviousMovementMode);
    }
    
    if (!IsActive())
    {
        Activate();
    }
}

void USFEnemyMovementComponent::RegisterStateTagEvents(UAbilitySystemComponent* ASC)
{
    if (!ASC) return;

    // 모든 CC 태그 등록
    ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Parried, EGameplayTagEventType::NewOrRemoved)
        .AddUObject(this, &ThisClass::OnStateTagChanged);

    ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Stunned, EGameplayTagEventType::NewOrRemoved)
        .AddUObject(this, &ThisClass::OnStateTagChanged);

    ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Knockback, EGameplayTagEventType::NewOrRemoved)
        .AddUObject(this, &ThisClass::OnStateTagChanged);

    ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Knockdown, EGameplayTagEventType::NewOrRemoved)
        .AddUObject(this, &ThisClass::OnStateTagChanged);

    ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Groggy, EGameplayTagEventType::NewOrRemoved)
        .AddUObject(this, &ThisClass::OnStateTagChanged);

    ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Hit, EGameplayTagEventType::NewOrRemoved)
        .AddUObject(this, &ThisClass::OnStateTagChanged);

    ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Dead, EGameplayTagEventType::NewOrRemoved)
        .AddUObject(this, &ThisClass::OnStateTagChanged);
}

void USFEnemyMovementComponent::OnStateTagChanged(const FGameplayTag Tag, int32 NewCount)
{
    if (!GetOwner()->HasAuthority()) return;

    if (NewCount > 0)
    {
        OnStateStart(Tag);
    }
    else
    {
        OnStateEnd(Tag);
    }
}

void USFEnemyMovementComponent::OnStateStart(FGameplayTag StateTag)
{
    if (!GetOwner()->HasAuthority())
    {
       return;
    }

    if (StateStartMap.Contains(StateTag))
    {
       StateStartMap[StateTag]();
    }
}

void USFEnemyMovementComponent::OnStateEnd(FGameplayTag StateTag)
{
    if (!GetOwner()->HasAuthority())
    {
       return;
    }

    if (StateEndMap.Contains(StateTag))
    {
       StateEndMap[StateTag]();
    }
}

void USFEnemyMovementComponent::StartStateParried()
{
    InternalDisableMovement();
}

void USFEnemyMovementComponent::EndStateParried()
{
    InternalEnableMovement();
}

void USFEnemyMovementComponent::StartStateStunned()
{
    InternalDisableMovement();
}

void USFEnemyMovementComponent::EndStateStunned()
{
    InternalEnableMovement();
}

void USFEnemyMovementComponent::StartStateGroggy()
{

    InternalDisableMovement();
}

void USFEnemyMovementComponent::EndStateGroggy()
{
    InternalEnableMovement();
}

void USFEnemyMovementComponent::StartStateDead()
{
    InternalDisableMovement();
}

void USFEnemyMovementComponent::EndStateDead()
{

}


void USFEnemyMovementComponent::StartStateKnockback()
{

}

void USFEnemyMovementComponent::EndStateKnockback()
{
    
    InternalEnableMovement();
}

void USFEnemyMovementComponent::StartStateKnockdown()
{
 
}

void USFEnemyMovementComponent::EndStateKnockdown()
{
    InternalEnableMovement();
}

void USFEnemyMovementComponent::StartStateHitReact()
{
 
}

void USFEnemyMovementComponent::EndStateHitReact()
{
    InternalEnableMovement();
}