// Fill out your copyright notice in the Description page of Project Settings.


#include "SFEnemyCombatComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Interface/SFEnemyAbilityInterface.h"


void USFEnemyCombatComponent::InitializeCombatComponent()  // component 초기화 
{
    // 초기화 //
}

bool USFEnemyCombatComponent::SelectAbility(
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

    float BestScore = -FLT_MAX;
    FGameplayTag BestTag;

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

        // Ability가 SearchTags 중 어떤 태그라도 포함하는가?
        if (!AllTags.HasAny(SearchTags))
        {
            continue;
        }

        const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();

        // 활성화 가능 체크
        if (!Ability->CanActivateAbility(Spec.Handle, ActorInfo))
        {
            continue;
        }

        // 쿨타임 체크
        if (!Ability->CheckCooldown(Spec.Handle, ActorInfo))
        {
            continue;
        }

        ISFEnemyAbilityInterface* AIInterface = Cast<ISFEnemyAbilityInterface>(Ability);
        if (!AIInterface)
        {
            continue;
        }

        // Context에 Spec 전달
        FEnemyAbilitySelectContext ContextWithSpec = Context;
        ContextWithSpec.AbilitySpec = &Spec;

        float Score = AIInterface->CalcAIScore(ContextWithSpec);

        if (Score > BestScore)
        {
            BestScore = Score;

            // SearchTags 와 정확히 매칭되는 태그만 추출
            FGameplayTag UniqueTag;
            for (const FGameplayTag& Tag : AllTags)
            {
                if (SearchTags.HasTagExact(Tag))
                {
                    UniqueTag = Tag;
                    break;
                }
            }

            // fallback — 그래도 못찾으면 그냥 AbilityTags 내 첫 태그 사용
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

            BestTag = UniqueTag;
        }
    }

    if (!BestTag.IsValid())
    {
        return false;
    }

    OutSelectedTag = BestTag;
    return true;
}
