// SFEnemyController.cpp
#include "SFEnemyController.h"

#include "Perception/AISense_Sight.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "Character/SFCharacterGameplayTags.h"
#include "BehaviorTree/BlackboardComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFEnemyController)

ASFEnemyController::ASFEnemyController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
    SetPerceptionComponent(*AIPerception);

    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = SightRadius;
    SightConfig->LoseSightRadius = LoseSightRadius;
    SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;

    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

    SightConfig->SetMaxAge(5.0f);
    SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.f;

    AIPerception->ConfigureSense(*SightConfig);
    AIPerception->SetDominantSense(UAISense_Sight::StaticClass());

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;

    CombatComponent = ObjectInitializer.CreateDefaultSubobject<USFEnemyCombatComponent>(this, TEXT("CombatComponent"));
}

void ASFEnemyController::InitializeAIController()
{
    Super::InitializeAIController();

    if (HasAuthority())
    {
        // CombatComponent의 전투 상태 변경 이벤트 구독
        if (CombatComponent)
        {
            CombatComponent->OnCombatStateChanged.AddDynamic(
                this, &ASFEnemyController::OnCombatStateChanged
            );
        }

    }
}

//  전투 상태 변경 콜백 
void ASFEnemyController::OnCombatStateChanged(bool bInCombat)
{
    bIsInCombat = bInCombat;
    
    // 시야각 조정
    if (SightConfig && AIPerception)
    {
        SightConfig->PeripheralVisionAngleDegrees = bInCombat ? 180.f : PeripheralVisionAngleDegrees;
        AIPerception->ConfigureSense(*SightConfig);

    }


    if (bInCombat)
    {
        SetRotationMode(EAIRotationMode::ControllerYaw);
    }
    else
    {
        SetRotationMode(EAIRotationMode::MovementDirection);
    }

    if (CachedBlackboardComponent)
    {
        CachedBlackboardComponent->SetValueAsBool("bIsInCombat", bInCombat);
    }
}

// 강제 타겟 설정 함수
void ASFEnemyController::SetTargetForce(AActor* NewTarget)
{
    if (!NewTarget)
    {
        return;
    }

    // 타겟 태그 확인
    if (!TargetTag.IsNone() && !NewTarget->ActorHasTag(TargetTag))
    {
        return;
    }

    if (CombatComponent)
    {
        CombatComponent->UpdateTargetActor(NewTarget);

    }
}