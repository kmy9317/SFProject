#include "SFEnemyController.h"

#include "Perception/AISense_Sight.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "Character/SFCharacterGameplayTags.h"
#include "BehaviorTree/BlackboardComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFEnemyController)


static TAutoConsoleVariable<bool> CVarShowAIDebug(
    TEXT("AI.ShowDebug"),
    false,
    TEXT("AI 감지 범위 및 시각화 디버그 표시\n")
    TEXT("0: 끔, 1: 켬"),
    ECVF_Default
);


ASFEnemyController::ASFEnemyController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // AI Perception 생성
    AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
    SetPerceptionComponent(*AIPerception);

    // 시야 감각 설정 생성 및 기본값 적용
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = SightRadius;
    SightConfig->LoseSightRadius = LoseSightRadius;
    SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;

    // 감지할 대상 유형
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

    // 시야 감지 지속 시간
    SightConfig->SetMaxAge(5.0f);
    SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.f;

    // 시야 감각 설정
    AIPerception->ConfigureSense(*SightConfig);
    AIPerception->SetDominantSense(UAISense_Sight::StaticClass());

    // Tick 활성화
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;

    // CombatComponent 생성
    CombatComponent = ObjectInitializer.CreateDefaultSubobject<USFEnemyCombatComponent>(this, TEXT("CombatComponent"));
}


void ASFEnemyController::InitializeAIController()
{
    // 베이스 클래스의 공통 초기화 호출
    Super::InitializeAIController();

    if (HasAuthority())
    {
        // AIPerception 바인딩 (Enemy 전용)
        if (AIPerception)
        {
            AIPerception->OnTargetPerceptionUpdated.AddDynamic(
                this, &ASFEnemyController::OnTargetPerceptionUpdated
            );

            AIPerception->OnTargetPerceptionForgotten.AddDynamic(
                this, &ASFEnemyController::OnTargetPerceptionForgotten
            );
        }
    }
}

void ASFEnemyController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    DrawDebugPerception();
}

void ASFEnemyController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor) return;

    if (!Stimulus.Type.IsValid() || Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>())
        return;

    if (!TargetTag.IsNone() && !Actor->ActorHasTag(TargetTag))
        return;

    //  CombatComponent에게 시야 감지 결과 전달
    if (CombatComponent)
    {
        CombatComponent->HandleTargetPerceptionUpdated(Actor, Stimulus.WasSuccessfullySensed());
    }

    // 감지 성공
    if (Stimulus.WasSuccessfullySensed())
    {
        // [수정됨] 직접 할당보다는 BTService가 최종 결정하도록 유도하지만,
        // 최초 발견 반응을 위해 변수는 유지하거나 블랙보드 갱신에 집중합니다.
        TargetActor = Actor;

        if (!bIsInCombat)
        {
            bIsInCombat = true;
            SightConfig->PeripheralVisionAngleDegrees = 180.f;
            AIPerception->ConfigureSense(*SightConfig);
        }

        // [삭제됨] 직접 SetFocus 호출 -> 이제 BTService (SFBTS_UpdateFocus)가 담당함
        // SetFocus(Actor, EAIFocusPriority::Gameplay); 

        if (CachedBlackboardComponent)
        {
            // 타겟을 발견했으므로 블랙보드 갱신 (추격 시작)
            CachedBlackboardComponent->SetValueAsObject("TargetActor", Actor);
            CachedBlackboardComponent->SetValueAsBool("bHasTarget", true);
            CachedBlackboardComponent->SetValueAsBool("bIsInCombat", true);
            CachedBlackboardComponent->SetValueAsVector("LastKnownPosition", Stimulus.StimulusLocation);
        }
    }
    // 감지 상실
    else
    {
        // [중요 수정] 여기서 타겟을 nullptr로 지우는 로직을 전면 삭제합니다.
        // 타겟 해제 판단은 이제 'BTService_UpdateTarget'에서 거리와 시간을 계산해서 수행합니다.
        
        // (디버깅용 로그 정도는 남길 수 있음)
        // UE_LOG(LogTemp, Verbose, TEXT("Perception Lost: %s (Service checking distance...)"), *GetNameSafe(Actor));
        
        /* -- 삭제된 코드 --
        TArray<AActor*> PerceivedActors;
        if (AIPerception) { ... }
        int32 PlayerCount = 0;
        for (AActor* PerceivedActor : PerceivedActors) { ... }

        if (PlayerCount == 0 && bIsInCombat)
        {
            bIsInCombat = false;
            TargetActor = nullptr; // <--- 삭제됨
            // ... 시야각 복구 로직 등도 Service 혹은 별도 State 처리 권장
            
            if (CachedBlackboardComponent)
            {
                CachedBlackboardComponent->ClearValue("TargetActor"); // <--- 삭제됨
                // ...
            }
        }
        */
    }
}

void ASFEnemyController::OnTargetPerceptionForgotten(AActor* Actor)
{
    // [중요 수정] 
    // 타겟 해제 판단(Give Up Logic)은 이제 'BTService_UpdateTarget'으로 완전히 이관되었습니다.
    // 따라서 Perception 시스템이 타겟을 잊었다고(Max Age 경과) 알려줘도,
    // 컨트롤러가 독단적으로 타겟 정보를 지우거나 전투 상태를 해제하지 않습니다.

    /* -- 전면 삭제 --
    if (!Actor) return;
    if (!TargetTag.IsNone() && !Actor->ActorHasTag(TargetTag)) return;
    
    // ... PlayerCount 계산 로직 ...

    if (PlayerCount == 0 && bIsInCombat)
    {
        bIsInCombat = false;
        TargetActor = nullptr;
        // ... 시야각 복구 ...
        // ... 블랙보드 초기화 ...
    }
    */
    
    // 필요하다면 디버그용 로그만 남겨둡니다.
    // UE_LOG(LogTemp, Verbose, TEXT("Perception Forgotten: %s"), *GetNameSafe(Actor));
}

void ASFEnemyController::DrawDebugPerception()
{
#if !UE_BUILD_SHIPPING
    const bool bShowDebug = CVarShowAIDebug.GetValueOnGameThread();
    if (!bShowDebug) return;

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

    const FVector PawnLocation = ControlledPawn->GetActorLocation();
    const FRotator PawnRotation = ControlledPawn->GetActorRotation();
    const FVector ForwardVector = PawnRotation.Vector();
    const float LifeTime = 0.f;

    FVector EyeLocation = PawnLocation;
    if (ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
        EyeLocation.Z += ControlledCharacter->BaseEyeHeight;
    else
        EyeLocation.Z += 90.f;

    if (!bIsInCombat)
    {
        DrawDebugCone(
            GetWorld(), EyeLocation, ForwardVector, SightRadius,
            FMath::DegreesToRadians(PeripheralVisionAngleDegrees),
            FMath::DegreesToRadians(PeripheralVisionAngleDegrees),
            16, FColor::Green, false, LifeTime, 0, 2.0f
        );

        DrawDebugString(GetWorld(), PawnLocation + FVector(0, 0, 150), TEXT("STATE: IDLE"), nullptr, FColor::White, LifeTime, true, 1.5f);
    }
    else
    {
        DrawDebugCone(
            GetWorld(), EyeLocation, ForwardVector, SightRadius,
            FMath::DegreesToRadians(180.f),
            FMath::DegreesToRadians(180.f),
            16, FColor::Cyan, false, LifeTime, 0, 1.5f
        );

        DrawDebugSphere(GetWorld(), PawnLocation, 200.f, 12, FColor::Red, false, LifeTime, 0, 4.0f); // Melee (임시)
        DrawDebugSphere(GetWorld(), PawnLocation, 1200.f, 16, FColor::Yellow, false, LifeTime, 0, 2.5f); // Guard (임시)

        if (TargetActor)
        {
            DrawDebugLine(
                GetWorld(), PawnLocation + FVector(0, 0, 50),
                TargetActor->GetActorLocation() + FVector(0, 0, 50),
                FColor::Cyan, false, LifeTime, 0, 2.0f
            );
        }
    }
#endif
}

// [추가] 강제 타겟 설정 함수 구현
void ASFEnemyController::SetTargetForce(AActor* NewTarget)
{
    // 유효하지 않거나 이미 같은 타겟이면 리턴
    if (!NewTarget || TargetActor == NewTarget)
    {
        return;
    }

    // 1. 내부 타겟 변수 업데이트
    TargetActor = NewTarget;
    
    // 2. 전투 상태(Combat)가 아니었다면 즉시 전환 및 시야 확장
    if (!bIsInCombat)
    {
        bIsInCombat = true;
        
        // 전투 중 시야각 확장 (180도)
        if (SightConfig)
        {
            SightConfig->PeripheralVisionAngleDegrees = 180.f; 
            if (AIPerception)
            {
                AIPerception->ConfigureSense(*SightConfig);
            }
        }
    }

    // 3. 블랙보드 값 즉시 업데이트 (Behavior Tree 반응 속도 향상)
    if (CachedBlackboardComponent)
    {
        CachedBlackboardComponent->SetValueAsObject("TargetActor", NewTarget);
        CachedBlackboardComponent->SetValueAsBool("bHasTarget", true);
        CachedBlackboardComponent->SetValueAsBool("bIsInCombat", true);
        
        // 추격 등을 위해 마지막 위치도 업데이트
        CachedBlackboardComponent->SetValueAsVector("LastKnownPosition", NewTarget->GetActorLocation());
    }

    // [중요 수정] 여기서 SetFocus를 직접 호출하지 않습니다!
    // SetFocus(NewTarget); <--- 이 줄이 있으면 피격 순간 몸이 획 돌아버립니다.
    // 이제 회전은 BT의 'SF Rotate to Target' 태스크나 'UpdateFocus' 서비스가 부드럽게 처리합니다.

    // 5. CombatComponent에도 알림 (거리 계산, 공격 가능 여부 판단 등을 위해 필수)
    if (CombatComponent)
    {
        // 강제로 감지된 것으로 처리하여 내부 상태 갱신
        CombatComponent->HandleTargetPerceptionUpdated(NewTarget, true);
    }
}


