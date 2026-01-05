// Fill out your copyright notice in the Description page of Project Settings.

#include "SFCombatComponentBase.h"
#include "AIController.h"
#include "AbilitySystemGlobals.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AI/SFAIGameplayTags.h"
#include "AI/Controller/SFBaseAIController.h"
#include "Character/SFCharacterBase.h"
#include "Interface/SFEnemyAbilityInterface.h"

USFCombatComponentBase::USFCombatComponentBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void USFCombatComponentBase::InitializeCombatComponent()
{
    AAIController* AIC = GetController<AAIController>();
    if (!AIC) return;

    APawn* Pawn = AIC->GetPawn();
    if (!Pawn) return;

    CachedASC = Cast<USFAbilitySystemComponent>(
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn));
    
}

void USFCombatComponentBase::UpdateTargetActor(AActor* NewTarget)
{
    if (CurrentTarget == NewTarget) return;

    AAIController* AIC = GetController<AAIController>();
    if (!AIC) return;

    bool bWasInCombat = (CurrentTarget != nullptr);
    CurrentTarget = NewTarget;
    if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
    {
        BB->SetValueAsObject("TargetActor", NewTarget);
        BB->SetValueAsBool("bHasTarget", NewTarget != nullptr);
    }
    
    if (ASFBaseAIController* SFAIC = Cast<ASFBaseAIController>(AIC))
    {
        SFAIC->TargetActor = NewTarget;

        if (NewTarget)
        {
            SFAIC->SetFocus(NewTarget, EAIFocusPriority::Gameplay);
        }
        else
        {
            SFAIC->ClearFocus(EAIFocusPriority::Gameplay);
        }
    }

    bool bNowInCombat = (NewTarget != nullptr);
    SetGameplayTagStatus(SFGameplayTags::AI_State_Combat, bNowInCombat);
    
    if (bWasInCombat != bNowInCombat)
    {
        OnCombatStateChanged.Broadcast(bNowInCombat);
    }
}

bool USFCombatComponentBase::SelectAbility(
    const FEnemyAbilitySelectContext& Context,
    const FGameplayTagContainer& SearchTags,
    FGameplayTag& OutSelectedTag)
{
    OutSelectedTag = FGameplayTag();

    UAbilitySystemComponent* ASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Context.Self);

    if (!ASC || SearchTags.IsEmpty())
    {
        return false;
    }
    
    FEnemyAbilitySelectContext ContextWithSpatialData = Context;

    if (Context.Self && Context.Target)
    {
        if (Context.DistanceToTarget == 0.f)
        {
            ContextWithSpatialData.DistanceToTarget = Context.Self->GetDistanceTo(Context.Target);
        }
        if (Context.AngleToTarget == 0.f)
        {
            if (ASFCharacterBase* Owner = Cast<ASFCharacterBase>(Context.Self))
            {
                const FVector ToTarget = (Context.Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
                const float Dot = FVector::DotProduct(Owner->GetActorForwardVector(), ToTarget);
                ContextWithSpatialData.AngleToTarget = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
            }
        }
    }

    
    TArray<FGameplayTag> Candidates;
    TArray<float> Weights;
    float TotalWeight = 0.f;

    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        UGameplayAbility* Ability = Spec.Ability;
        if (!Ability)
        {
            continue;
        }
        
        FGameplayTagContainer AllTags;
        AllTags.AppendTags(Ability->AbilityTags);
        AllTags.AppendTags(Ability->GetAssetTags());

        
        if (!AllTags.HasAny(SearchTags))
        {
            continue;
        }

        const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();

        
        if (!Ability->CheckCooldown(Spec.Handle, ActorInfo))
        {
            continue;
        }

        
        ISFEnemyAbilityInterface* AIInterface = Cast<ISFEnemyAbilityInterface>(Ability);
        if (!AIInterface)
        {
            continue;
        }

        
        FEnemyAbilitySelectContext ContextWithSpec = ContextWithSpatialData;
        ContextWithSpec.AbilitySpec = &Spec;

        float Score = AIInterface->CalcAIScore(ContextWithSpec);

        if (Score > 0.f)
        {
           
            FGameplayTag UniqueTag;
            for (const FGameplayTag& Tag : AllTags)
            {
                if (SearchTags.HasTagExact(Tag))
                {
                    UniqueTag = Tag;
                    break;
                }
            }

            if (!UniqueTag.IsValid())
            {
                if (Ability->AbilityTags.Num() > 0)
                {
                    UniqueTag = Ability->AbilityTags.First();
                }
                else if (Ability->GetAssetTags().Num() > 0)
                {
                    UniqueTag = Ability->GetAssetTags().First();
                }
            }

            if (UniqueTag.IsValid())
            {
                Candidates.Add(UniqueTag);
                Weights.Add(Score);
                TotalWeight += Score;
            }
        }
    }

    // No candidates
    if (Candidates.Num() == 0)
    {
        return false;
    }

    // Weighted random selection
    float RandomValue = FMath::FRandRange(0.f, TotalWeight);

    for (int32 i = 0; i < Candidates.Num(); ++i)
    {
        if (RandomValue <= Weights[i])
        {
            OutSelectedTag = Candidates[i];
            return true;
        }
        RandomValue -= Weights[i];
    }

    // Fallback to last candidate
    OutSelectedTag = Candidates.Last();
    return true;
}

void USFCombatComponentBase::SetGameplayTagStatus(const FGameplayTag& Tag, bool bActive)
{
    if (!CachedASC || !Tag.IsValid()) return;

    if (bActive)
    {
        if (!CachedASC->HasMatchingGameplayTag(Tag))
            CachedASC->AddLooseGameplayTag(Tag);
    }
    else
    {
        if (CachedASC->HasMatchingGameplayTag(Tag))
            CachedASC->RemoveLooseGameplayTag(Tag);
    }
}

APawn* USFCombatComponentBase::GetOwnerPawn() const
{
    if (AAIController* AIC = GetController<AAIController>())
        return AIC->GetPawn();
    return nullptr;
}

