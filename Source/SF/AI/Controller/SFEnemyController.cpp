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
    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
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
        }
    }
}



void ASFEnemyController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor) return;

    // 시야(Sight) 감각인지 확인
    if (!Stimulus.Type.IsValid() || Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>())
        return;

    // 타겟 태그 확인
    if (!TargetTag.IsNone() && !Actor->ActorHasTag(TargetTag))
        return;

    // CombatComponent에게 알림
    if (CombatComponent)
    {
        CombatComponent->HandleTargetPerceptionUpdated(Actor, Stimulus.WasSuccessfullySensed());
    }

    // 감지 성공 시
    if (Stimulus.WasSuccessfullySensed())
    {
        TargetActor = Actor;

        // 전투 상태가 아니었다면 전환
        if (!bIsInCombat)
        {
            bIsInCombat = true;
            SightConfig->PeripheralVisionAngleDegrees = 180.f;
            AIPerception->ConfigureSense(*SightConfig);

            // 전투 시작 시 ControllerYaw 모드로 전환 (부드러운 회전)
            SetRotationMode(EAIRotationMode::ControllerYaw);
        }

        if (CachedBlackboardComponent)
        {
            CachedBlackboardComponent->SetValueAsObject("TargetActor", Actor);
            CachedBlackboardComponent->SetValueAsBool("bHasTarget", true);
            CachedBlackboardComponent->SetValueAsBool("bIsInCombat", true);
            CachedBlackboardComponent->SetValueAsVector("LastKnownPosition", Stimulus.StimulusLocation);
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[%s] 시야 상실: %s"), *GetName(), *GetNameSafe(Actor));

        // 시야 상실 시 MovementDirection으로 복귀
        SetRotationMode(EAIRotationMode::MovementDirection);
    }
}




//강제 타겟 설정 함수
void ASFEnemyController::SetTargetForce(AActor* NewTarget)
{
    if (!NewTarget || TargetActor == NewTarget)
    {
        return;
    }

    if (!TargetTag.IsNone() && !NewTarget->ActorHasTag(TargetTag))
    {
        return;
    }
    
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


