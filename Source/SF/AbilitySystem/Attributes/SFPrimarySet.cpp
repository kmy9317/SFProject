// Fill out your copyright notice in the Description page of Project Settings.

#include "SFPrimarySet.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystem/GameplayCues/SFGameplayCueTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Libraries/SFAbilitySystemLibrary.h"
#include "Character/SFCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/SFPlayerState.h"
#include "Player/Components/SFPlayerStatsComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "UI/InGame/UIDataStructs.h"

USFPrimarySet::USFPrimarySet()
{
}

void USFPrimarySet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MoveSpeed, COND_None, REPNOTIFY_Always);
    
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MoveSpeedPercent, COND_None, REPNOTIFY_Always);
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

        if (SFASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Invulnerable))
        {
            SetDamage(0.0f); // 데미지 무효화
            return;
        }
        
        const float DamageDone = GetDamage();
        SetDamage(0.0f);
        
        if (DamageDone <= 0.0f) 
        {
            return;
        }

        if (AActor* OwnerActor = GetOwningActor())
        {
            if (OwnerActor->HasAuthority())
            {
                TrackDamageDealt(Data, DamageDone);
            }
        }
        
        // Parry Check
        if (SFASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Parrying))
        {
            USFAbilitySystemLibrary::SendParryEventFromSpec(SFASC, DamageDone, Data.EffectSpec);
            return;
        }
        
        // Apply damage
        const float NewHealth = GetHealth() - DamageDone;
        SetHealth(NewHealth);

        // 데미지 스크린 알림
        if (USFPlayerCombatStateComponent* CombatComp = USFPlayerCombatStateComponent::FindPlayerCombatStateComponent(GetOwningActor()))
        {
            CombatComp->NotifyDamageReceived(DamageDone);
        }

        // [UI] 데미지 폰트 띄우기 메시지
        if (DamageDone > 0.0f)
        {
            FGameplayCueParameters Params;
            Params.RawMagnitude = DamageDone;
            Params.EffectContext = Data.EffectSpec.GetContext();
            SFASC->ExecuteGameplayCue(SFGameplayTags::GameplayCue_Character_DamageTaken,Params) ;
        }
        
        if (NewHealth > 0)
        {
            USFAbilitySystemLibrary::SendHitReactionEventFromSpec(SFASC, DamageDone, Data.EffectSpec);
        }
        else 
        {
            if (AActor* OwnerActor = GetOwningActor())
            {
                if (OwnerActor->HasAuthority())
                {
                    //1회 부활 기능(색욕 30레벨) 로직
                    const FGameplayTag LastStandAvailableTag =
                    FGameplayTag::RequestGameplayTag(TEXT("Ability.Skill.Passive.LastStand"));
                    const FGameplayTag LastStandUsedTag =
                    FGameplayTag::RequestGameplayTag(TEXT("Ability.Skill.Passive.LastStand.Use"));
                    if (SFASC->HasMatchingGameplayTag(LastStandAvailableTag) &&
                        !SFASC->HasMatchingGameplayTag(LastStandUsedTag))
                    {
                        FGameplayEventData EventData;
                        EventData.EventTag = SFGameplayTags::GameplayEvent_PlayerAbility_LastStand;

                        SFASC->HandleGameplayEvent(
                        EventData.EventTag,
                        &EventData
                        );

                        return;
                    }
                    
                    //사망 이벤트
                    if (!SFASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
                    {
                        HandleZeroHealth(SFASC, Data);
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
    
    if (Attribute == GetMoveSpeedAttribute())
    {
        // 1. 값이 변했는지 확인
        UE_LOG(LogTemp, Warning, TEXT("[SFPrimarySet] MoveSpeed Changed! Old: %f -> New: %f"), OldValue, NewValue);

        if (ASFCharacterBase* Character = Cast<ASFCharacterBase>(GetOwningActor()))
        {
            if (UCharacterMovementComponent* CMC = Character->GetCharacterMovement())
            {
                // 2. 실제 캐릭터 속도에 반영되었는지 확인
                CMC->MaxWalkSpeed = NewValue;
                UE_LOG(LogTemp, Warning, TEXT("[SFPrimarySet] CMC MaxWalkSpeed Updated to: %f"), CMC->MaxWalkSpeed);
            }
        }
    }
}

void USFPrimarySet::HandleZeroHealth(USFAbilitySystemComponent* SFASC, const FGameplayEffectModCallbackData& Data)
{
    USFAbilitySystemLibrary::SendDeathEventFromSpec(SFASC, Data.EffectSpec);
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

void USFPrimarySet::TrackDamageDealt(const FGameplayEffectModCallbackData& Data, float DamageAmount)
{
    // Instigator 찾기
    AActor* Instigator = Data.EffectSpec.GetContext().GetInstigator();
    if (!Instigator)
    {
        return;
    }

    ASFPlayerState* InstigatorPS = nullptr;

    // Instigator가 PlayerState인 경우
    if (ASFPlayerState* PS = Cast<ASFPlayerState>(Instigator))
    {
        InstigatorPS = PS;
    }
    // Instigator가 Pawn인 경우
    else if (APawn* InstigatorPawn = Cast<APawn>(Instigator))
    {
        InstigatorPS = InstigatorPawn->GetPlayerState<ASFPlayerState>();
    }

    if (!InstigatorPS)
    {
        return;
    }

    if (USFPlayerStatsComponent* StatsComp = USFPlayerStatsComponent::FindPlayerStatsComponent(InstigatorPS))
    {
        StatsComp->AddDamageDealt(DamageAmount);
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
    if (ASFCharacterBase* Character = Cast<ASFCharacterBase>(GetOwningActor()))
    {
        if (UCharacterMovementComponent* CMC = Character->GetCharacterMovement())
        {
            CMC->MaxWalkSpeed = MoveSpeed.GetCurrentValue();
        }
    }
}

void USFPrimarySet::OnRep_MoveSpeedPercent(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MoveSpeedPercent, OldValue);
}
