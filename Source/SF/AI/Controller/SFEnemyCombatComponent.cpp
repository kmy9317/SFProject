#include "SFEnemyCombatComponent.h"
#include "TimerManager.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "SFBaseAIController.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Enemy/SFCombatSet_Enemy.h"
#include "Interface/SFEnemyAbilityInterface.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "AI/SFAIGameplayTags.h"
#include "Character/SFCharacterBase.h"
#include "BehaviorTree/BlackboardComponent.h"

USFEnemyCombatComponent::USFEnemyCombatComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void USFEnemyCombatComponent::InitializeCombatComponent()
{
    AAIController* AIC = GetController<AAIController>();
    if (!AIC) return;

    APawn* Pawn = AIC->GetPawn();
    if (!Pawn) return;

    CachedASC = Cast<USFAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn));
    
    if (!CachedASC) 
    { 
        UE_LOG(LogTemp, Error, TEXT("❌ [CombatComp] CachedASC is NULL!")); 
        return;
    }

    const UAttributeSet* Set = CachedASC->GetAttributeSet(USFCombatSet_Enemy::StaticClass());
    CachedCombatSet = const_cast<USFCombatSet_Enemy*>(Cast<USFCombatSet_Enemy>(Set));
    
    if (CachedCombatSet)
    {
        UpdatePerceptionConfig();
    }

    if (UAIPerceptionComponent* PerceptionComp = AIC->GetPerceptionComponent())
    {
        PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &USFEnemyCombatComponent::OnTargetPerceptionUpdated);
        PerceptionComp->OnPerceptionUpdated.AddDynamic(this, &USFEnemyCombatComponent::OnPerceptionUpdated);
    }

    StartTargetEvaluationTimer();
}

void USFEnemyCombatComponent::StartTargetEvaluationTimer()
{
    AAIController* AIC = GetController<AAIController>();
    if (!AIC) return;

    UWorld* World = AIC->GetWorld();
    if (!World) return;

    if (TargetEvaluationTimerHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(TargetEvaluationTimerHandle);
    }

    World->GetTimerManager().SetTimer(
        TargetEvaluationTimerHandle,
        this,
        &USFEnemyCombatComponent::OnTargetEvaluationTimer,
        TargetEvaluationInterval,
        true
    );
}

void USFEnemyCombatComponent::StopTargetEvaluationTimer()
{
    AAIController* AIC = GetController<AAIController>();
    if (!AIC) return;

    UWorld* World = AIC->GetWorld();
    if (!World) return;

    if (TargetEvaluationTimerHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(TargetEvaluationTimerHandle);
        TargetEvaluationTimerHandle.Invalidate();
    }
}

void USFEnemyCombatComponent::OnTargetEvaluationTimer()
{
    EvaluateBestTarget();
    UpdateCombatRangeTags();
}

void USFEnemyCombatComponent::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor || !Actor->ActorHasTag(FName("Player"))) return;

    AAIController* AIC = GetController<AAIController>();
    if (!AIC) return;
    
    if (CurrentTarget && CurrentTarget != Actor)
    {
        if (ASFBaseAIController* SFAIC = Cast<ASFBaseAIController>(AIC))
        {
            if (SFAIC->IsTurningInPlace()) return;
        }
    }

    if (Stimulus.WasSuccessfullySensed())
    {
        EvaluateBestTarget();
    }
    else if (CurrentTarget == Actor)
    {
        EvaluateBestTarget();
    }
}

void USFEnemyCombatComponent::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
    EvaluateBestTarget();
}

void USFEnemyCombatComponent::EvaluateBestTarget()
{
    AAIController* AIC = GetController<AAIController>();
    if (!AIC) return;

    UAIPerceptionComponent* PerceptionComp = AIC->GetPerceptionComponent();
    if (!PerceptionComp) return;

    TArray<AActor*> PerceivedActors;
    PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

    AActor* BestTarget = nullptr;
    float BestScore = -1.f;

    for (AActor* Actor : PerceivedActors)
    {
        if (!Actor->ActorHasTag(FName("Player"))) continue;

        float Score = CalculateTargetScore(Actor);
        if (Score > BestScore)
        {
            BestScore = Score;
            BestTarget = Actor;
        }
    }

    UpdateTargetActor(BestTarget);
}

float USFEnemyCombatComponent::CalculateTargetScore(AActor* Target) const
{
    if (!Target) return -1.f;
    APawn* MyPawn = GetOwnerPawn();
    if (!MyPawn) return -1.f;

    const float Distance = FVector::Dist(MyPawn->GetActorLocation(), Target->GetActorLocation());
    float Score = FMath::Clamp(1000.f - (Distance / 10.f), 0.f, 1000.f);

    FVector ToTarget = (Target->GetActorLocation() - MyPawn->GetActorLocation()).GetSafeNormal();
    float Dot = FVector::DotProduct(MyPawn->GetActorForwardVector(), ToTarget);
    Score += (Dot * 100.f);

    return Score;
}

void USFEnemyCombatComponent::UpdateTargetActor(AActor* NewTarget)
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
            SFAIC->SetRotationMode(EAIRotationMode::ControllerYaw);
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
            UE_LOG(LogTemp, Verbose, TEXT("[SelectAbility] Ability on cooldown: %s"), 
                   *Ability->GetName());
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
            // 대표 Tag 찾기 
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

    //  후보가 없으면 실패
    if (Candidates.Num() == 0)
    {
        return false;
    }

    // 가중치 기반 랜덤 선택 
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


    OutSelectedTag = Candidates.Last();
    return true;
}

void USFEnemyCombatComponent::UpdateCombatRangeTags()
{
    if (!CachedASC || !CachedCombatSet || !CurrentTarget) 
    {
        SetGameplayTagStatus(SFGameplayTags::AI_Range_Melee, false);
        SetGameplayTagStatus(SFGameplayTags::AI_Range_Guard, false);
        return;
    }

    APawn* MyPawn = GetOwnerPawn();
    if (!MyPawn) return;

    float Distance = FVector::Dist(MyPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
    
    SetGameplayTagStatus(SFGameplayTags::AI_Range_Melee, Distance <= CachedCombatSet->GetMeleeRange());
    SetGameplayTagStatus(SFGameplayTags::AI_Range_Guard, Distance <= CachedCombatSet->GetGuardRange());
}

void USFEnemyCombatComponent::UpdatePerceptionConfig()
{
    if (!CachedCombatSet) return;
    
    AAIController* AIC = Cast<AAIController>(GetOwner());
    if (!AIC || !AIC->GetPerceptionComponent()) return;

    UAIPerceptionComponent* Perception = AIC->GetPerceptionComponent();
    FAISenseID SightID = UAISense::GetSenseID<UAISense_Sight>();
    UAISenseConfig_Sight* SightConfig = Cast<UAISenseConfig_Sight>(Perception->GetSenseConfig(SightID));

    if (SightConfig)
    {
        SightConfig->SightRadius = CachedCombatSet->GetSightRadius();
        SightConfig->LoseSightRadius = CachedCombatSet->GetLoseSightRadius();
        Perception->ConfigureSense(*SightConfig);
    }
}

void USFEnemyCombatComponent::SetGameplayTagStatus(const FGameplayTag& Tag, bool bActive)
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

APawn* USFEnemyCombatComponent::GetOwnerPawn() const
{
    if (AAIController* AIC = GetController<AAIController>()) 
        return AIC->GetPawn();
    return nullptr;
}