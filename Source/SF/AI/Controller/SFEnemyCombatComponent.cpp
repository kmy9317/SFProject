// Fill out your copyright notice in the Description page of Project Settings.


#include "SFEnemyCombatComponent.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Enemy/SFCombatSet_Enemy.h"
#include "Interface/SFEnemyAbilityInterface.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "AI/SFAIGameplayTags.h"
USFEnemyCombatComponent::USFEnemyCombatComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Tick 활성화 (거리 계산용)
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void USFEnemyCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeCombatComponent();
}

void USFEnemyCombatComponent::InitializeCombatComponent()
{
	AAIController* AIC = GetController<AAIController>();
	if (!AIC) return;

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn) return;

	CachedASC = Cast<USFAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn));
    
	// [디버그] ASC 확인
	if (!CachedASC) { UE_LOG(LogTemp, Error, TEXT("❌ [CombatComp] CachedASC is NULL!")); }

	if (CachedASC)
	{
		// AttributeSet 가져오기
		const UAttributeSet* Set = CachedASC->GetAttributeSet(USFCombatSet_Enemy::StaticClass());
		CachedCombatSet = const_cast<USFCombatSet_Enemy*>(Cast<USFCombatSet_Enemy>(Set));
        
		// [디버그] 캐스팅 결과 확인
		if (CachedCombatSet)
		{
			UE_LOG(LogTemp, Warning, TEXT("✅ [CombatComp] CombatSet Init OK! MeleeRange: %f"), CachedCombatSet->GetMeleeRange());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("❌ [CombatComp] CombatSet Cast FAILED! (부모 클래스로 생성된 듯 함)"));
		}
	}

	UpdatePerceptionConfig();
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

//========================================================
//시야 로직
//========================================================

void USFEnemyCombatComponent::UpdatePerceptionConfig()
{
    if (!CachedCombatSet)
    {
        return;
    }

    float SightRadius = CachedCombatSet->GetSightRadius();
    float LoseSightRadius = CachedCombatSet->GetLoseSightRadius();

    if (AAIController* AIC = GetController<AAIController>())
    {
        if (UAIPerceptionComponent* Perception = AIC->GetPerceptionComponent())
        {
            // Sight 설정 가져와서 값 덮어쓰기
            if (UAISenseConfig_Sight* SightConfig = Cast<UAISenseConfig_Sight>(Perception->GetSenseConfig(UAISense::GetSenseID<UAISense_Sight>())))
            {
                bool bUpdated = false;
                if (SightRadius > 0.f)
                {
                    SightConfig->SightRadius = SightRadius;
                    bUpdated = true;
                }
                if (LoseSightRadius > 0.f)
                {
                    SightConfig->LoseSightRadius = LoseSightRadius;
                    bUpdated = true;
                }
				
                // 변경된 설정 적용
                if (bUpdated)
                {
                    Perception->ConfigureSense(*SightConfig);
                    Perception->RequestStimuliListenerUpdate();
					
                UE_LOG(LogTemp, Log, TEXT("[SFCombatComp] 시야 설정 업데이트 완료 - 감지거리: %.1f, 추적중단거리: %.1f"), SightRadius, LoseSightRadius);                }
            }
        }
    }
}

void USFEnemyCombatComponent::HandleTargetPerceptionUpdated(AActor* Actor, bool bSuccessfullySensed)
{
	// 타겟 감지 로직
	if (bSuccessfullySensed)
	{
		// 새로운 적 발견 -> 타겟 설정 및 전투 상태 태그 부여
		if (CurrentTarget != Actor)
		{
			CurrentTarget = Actor;
			SetGameplayTagStatus(SFGameplayTags::AI_State_Combat, true);
			UE_LOG(LogTemp, Log, TEXT("[SFCombatComp] 타겟 획득: %s"), *Actor->GetName());		}
	}
	else
	{
		// 시야 상실
		if (CurrentTarget == Actor)
		{
			// 바로 해제할지, 일정 시간 유지할지는 플레이해보고 결정 일단은 즉시 해지
			CurrentTarget = nullptr;
			SetGameplayTagStatus(SFGameplayTags::AI_State_Combat, false);
		}
	}
}

void USFEnemyCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 매 프레임 거리 체크 및 태그 갱신
	UpdateCombatRangeTags();
}

void USFEnemyCombatComponent::UpdateCombatRangeTags()
{
	// [디버그 1] 필수 데이터가 없는지 확인
	if (!CachedASC) { UE_LOG(LogTemp, Error, TEXT("DEBUG: CachedASC is NULL")); return; }
	if (!CachedCombatSet) { UE_LOG(LogTemp, Error, TEXT("DEBUG: CachedCombatSet is NULL")); return; }
    
	// [디버그 2] 타겟이 없는지 확인
	if (!CurrentTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("DEBUG: CurrentTarget is NULL (Tag Cleared)"));
		SetGameplayTagStatus(SFGameplayTags::AI_Range_Melee, false);
		SetGameplayTagStatus(SFGameplayTags::AI_Range_Guard, false);
		return;
	}

	float Distance = FVector::Dist(GetController<AAIController>()->GetPawn()->GetActorLocation(), CurrentTarget->GetActorLocation());
	float MeleeRange = CachedCombatSet->GetMeleeRange();
	float GuardRange = CachedCombatSet->GetGuardRange();

	// [디버그 3] 실제 값 확인 (이 로그가 핵심입니다)
	UE_LOG(LogTemp, Log, TEXT("DEBUG: Dist: %.2f / Melee: %.2f / Guard: %.2f / MeleeTag: %d"), 
		Distance, MeleeRange, GuardRange, (Distance <= MeleeRange));

	SetGameplayTagStatus(SFGameplayTags::AI_Range_Melee, Distance <= MeleeRange);
	SetGameplayTagStatus(SFGameplayTags::AI_Range_Guard, Distance <= GuardRange);
}

void USFEnemyCombatComponent::SetGameplayTagStatus(const FGameplayTag& Tag, bool bActive)
{
	if (!CachedASC || !Tag.IsValid()) return;

	if (bActive)
	{
		if (!CachedASC->HasMatchingGameplayTag(Tag))
		{
			CachedASC->AddLooseGameplayTag(Tag);
		}
	}
	else
	{
		if (CachedASC->HasMatchingGameplayTag(Tag))
		{
			CachedASC->RemoveLooseGameplayTag(Tag);
		}
	}
}