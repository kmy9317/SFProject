// Fill out your copyright notice in the Description page of Project Settings.

#include "SFPrimarySet.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterGameplayTags.h"




USFPrimarySet::USFPrimarySet()
{
}

void USFPrimarySet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MoveSpeed, COND_None, REPNOTIFY_Always);
}

bool USFPrimarySet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
    if (!Super::PreGameplayEffectExecute(Data))
    {
        return false;
    }

    if (Data.EvaluatedData.Attribute == GetDamageAttribute())
    {
        USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent();
        
        if (SFASC && SFASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
        {
            return false;
        }
        
        if (GetHealth() <= 0.0f)
        {
            return false;
        }
    }

    return true;
}

void USFPrimarySet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetDamageAttribute())
    {
        USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent();
        if (!SFASC)
        {
            return;
        }
        
        const float DamageDone = GetDamage();
        SetDamage(0.0f);
        
        if (DamageDone <= 0.0f) 
        {
            return;
        }
        
        // Parry Check
        if (SFASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Parrying))
        {
            SFASC->ProcessParryEvent(DamageDone, Data.EffectSpec);
            return;
        }
        
        // Apply damage
        const float NewHealth = GetHealth() - DamageDone;
        SetHealth(NewHealth);
        
        if (NewHealth > 0)
        {
            SFASC->ProcessHitReactionEvent(DamageDone, Data.EffectSpec);
        }
        else 
        {
            if (AActor* OwnerActor = GetOwningActor())
            {
                if (OwnerActor->HasAuthority())
                {
                    if (!SFASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
                    {
                        SFASC->ProcessDeathEvent(Data.EffectSpec);
                    }
                }
            }
        }
    }
}

void USFPrimarySet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void USFPrimarySet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void USFPrimarySet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);
    
    if (Attribute == GetMaxHealthAttribute())
    {
        if (GetHealth() > NewValue)
        {
            USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent();
            if (SFASC)
            {
                SFASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Override, NewValue);
            }
        }
    }
    
}

void USFPrimarySet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    else if (Attribute == GetMaxHealthAttribute())
    {
        NewValue = FMath::Max(NewValue, 1.0f);
    }
    else if (Attribute == GetMoveSpeedAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
}

void USFPrimarySet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Health, OldValue);
    
}

void USFPrimarySet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxHealth, OldValue);
}

void USFPrimarySet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MoveSpeed, OldValue);
}