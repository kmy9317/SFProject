// SFEnemyController.cpp
#include "SFEnemyController.h"

#include "Perception/AISense_Sight.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "Character/SFCharacterGameplayTags.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/CharacterMovementComponent.h"

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
    
    // 일반 적의 회전 속도 설정
    RotationInterpSpeed = 8.f; // Dragon보다 빠른 회전
}

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

void ASFEnemyController::InitializeAIController()
{
    Super::InitializeAIController();

    if (HasAuthority())
    {
        if (CombatComponent)
        {
            CombatComponent->OnCombatStateChanged.AddDynamic(
                this, &ASFEnemyController::OnCombatStateChanged
            );
        }
    }
}

void ASFEnemyController::OnCombatStateChanged(bool bInCombat)
{
    bIsInCombat = bInCombat;
    
    
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

void ASFEnemyController::RotateActorTowardsController(float DeltaTime)
{
    APawn* MyPawn = GetPawn();
    if (!MyPawn) return;

    FRotator ControllerRot = GetControlRotation();
    FRotator CurrentActorRot = MyPawn->GetActorRotation();
    
    
    FRotator TargetRot = FRotator(0.f, ControllerRot.Yaw, 0.f);

    float AppliedInterpSpeed = RotationInterpSpeed;
    
    bool bIsUsingAbility = false;
    if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MyPawn))
    {
        if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_UsingAbility))
        {
            bIsUsingAbility = true;
            AppliedInterpSpeed *= 0.2f; 
            
            float YawDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentActorRot.Yaw, TargetRot.Yaw));
            if (YawDiff < 15.f)
            {
                return; 
            }
        }
    }

    // 부드럽게 보간하여 회전
    FRotator NewActorRot = FMath::RInterpTo(CurrentActorRot, TargetRot, DeltaTime, AppliedInterpSpeed);
    MyPawn->SetActorRotation(NewActorRot);
}

bool ASFEnemyController::ShouldRotateActorByController() const
{
    return bIsInCombat;
}
float ASFEnemyController::GetTurnThreshold() const
{
    // 일반 적은 임계값이 필요 없음 (항상 부드럽게 회전)
    return 360.f;
}

bool ASFEnemyController::IsTurningInPlace() const
{
    return false;
}