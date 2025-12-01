// Fill out your copyright notice in the Description page of Project Settings.

#include "SFEnemyController.h"

#include "AI/StateMachine/SFStateMachine.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Perception/AISense_Sight.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "AI/Controller//SFEnemyCombatComponent.h"

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

    // 시야 감지 지속 시간 (중요: Forget Stale Actors 설정 필요)
    SightConfig->SetMaxAge(5.0f);
    SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.f;

    // 시야 감각 설정
    AIPerception->ConfigureSense(*SightConfig);
    AIPerception->SetDominantSense(UAISense_Sight::StaticClass());

    // Tick 활성화 (디버그용) 추후 false 예정
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;

    bIsInCombat = false;
    TargetActor = nullptr;

    //CombatComponent 앞으로 여기서 전투 관련 로직 개발 하도록 리팩토링 예정   ->  Controller에 만들어둔 것들 싹다 
    CombatComponent = ObjectInitializer.CreateDefaultSubobject<USFEnemyCombatComponent>(this, TEXT("CombatComponent")); 
}

//네트워크 복제 함수
void ASFEnemyController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASFEnemyController, bIsInCombat);
    DOREPLIFETIME(ASFEnemyController, bHasGuardSlot);

}

void ASFEnemyController::PreInitializeComponents()
{
    Super::PreInitializeComponents();

    UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}


void ASFEnemyController::BeginPlay()
{
    UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);
    Super::BeginPlay();

    // Perception 이벤트 바인딩
    if (AIPerception)
    {
        // 감지 갱신 (Sight In/Out)
        AIPerception->OnTargetPerceptionUpdated.AddDynamic(
            this, &ASFEnemyController::OnTargetPerceptionUpdated
        );
        
        //완전 소실 (MaxAge 경과)
        AIPerception->OnTargetPerceptionForgotten.AddDynamic(
            this, &ASFEnemyController::OnTargetPerceptionForgotten
        );

        UE_LOG(LogTemp, Log, TEXT("[SFEnemyAI] Perception 이벤트 바인딩 완료 (Updated + Forgotten)"));
    }

}



void ASFEnemyController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
    Super::EndPlay(EndPlayReason);
}



void ASFEnemyController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    
    if (!InPawn)
    {
        return;
    }


    // 스폰 위치 저장 (귀환 행동 등에 사용 가능)
    SpawnLocation = InPawn->GetActorLocation();
    UE_LOG(LogTemp, Log, TEXT("[SFEnemyAI] Spawn 위치 저장: %s"), *SpawnLocation.ToString());

    // 상태 머신 이벤트 바인딩
    BindingStateMachine(InPawn);
}


void ASFEnemyController::OnUnPossess()
{
    UnBindingStateMachine();
    Super::OnUnPossess();
}


void ASFEnemyController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    DrawDebugPerception();
}


void ASFEnemyController::SetBehaviorTree(UBehaviorTree* NewBehaviorTree)
{
    if (!NewBehaviorTree)
        return;
   
    if (CachedBehaviorTreeComponent && 
        CachedBehaviorTreeComponent->GetCurrentTree() == NewBehaviorTree)
    {
        return;
    }

    RunBehaviorTree(NewBehaviorTree);
}
void ASFEnemyController::BindingStateMachine(const APawn* InPawn)
{
    if (!InPawn)
        return;

    
    USFStateMachine* StateMachine = USFStateMachine::FindStateMachineComponent(InPawn);
    if (StateMachine)
    {
        StateMachine->OnChangeTreeDelegate.AddUObject(this, &ThisClass::ChangeBehaviorTree);
        StateMachine->OnStopTreeDelegate.AddUObject(this, &ThisClass::StopBehaviorTree);
    }
}

void ASFEnemyController::UnBindingStateMachine()
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
        return;

    USFStateMachine* StateMachine = USFStateMachine::FindStateMachineComponent(ControlledPawn);
    if (StateMachine)
    {
        StateMachine->OnChangeTreeDelegate.RemoveAll(this);
        StateMachine->OnStopTreeDelegate.RemoveAll(this);
    }
}

void ASFEnemyController::StopBehaviorTree()
{
    if (CachedBehaviorTreeComponent)
        CachedBehaviorTreeComponent->StopTree();
}

void ASFEnemyController::ChangeBehaviorTree(FGameplayTag GameplayTag)
{
    if (!GameplayTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SFEnemyAI] ChangeBehaviorTree 실패: 태그가 유효하지 않음"));
        return;
    }

    UBehaviorTree* NewTree = BehaviorTreeContainer.GetBehaviourTree(GameplayTag);
    if (NewTree)
    {
        UE_LOG(LogTemp, Log, TEXT("[SFEnemyAI] BehaviorTree 변경: %s"), *GameplayTag.ToString());
        SetBehaviorTree(NewTree);
    }
}

bool ASFEnemyController::RunBehaviorTree(UBehaviorTree* BehaviorTree)
{
    if (!BehaviorTree)
    {
        UE_LOG(LogTemp, Error, TEXT("[SFEnemyAI] RunBehaviorTree 실패: 전달된 트리가 nullptr"));
        return false;
    }

    const bool bSuccess = Super::RunBehaviorTree(BehaviorTree);

    
    if (bSuccess)
    {
        CachedBehaviorTreeComponent = Cast<UBehaviorTreeComponent>(GetBrainComponent());
        CachedBlackboardComponent = GetBlackboardComponent();

        UE_LOG(LogTemp, Log, TEXT("[SFEnemyAI] BehaviorTree 실행 및 컴포넌트 캐싱 완료: %s"), *BehaviorTree->GetName());
    }

    bIsInCombat = false;
    return bSuccess;
}



void ASFEnemyController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor) return;

    // 시야 감각만 처리
    if (!Stimulus.Type.IsValid() || Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>())
    {
        return;
    }

    // 타겟 태그 필터링
    if (!TargetTag.IsNone() && !Actor->ActorHasTag(TargetTag))
    {
        return;
    }

    // CombatComponent에게 처리 위임
    if (CombatComponent)
    {
        CombatComponent->HandleTargetPerceptionUpdated(Actor, Stimulus.WasSuccessfullySensed());
    }

    // Blackboard 업데이트
    if (Stimulus.WasSuccessfullySensed())
    {
        TargetActor = Actor;
        
        if (CachedBlackboardComponent)
        {
            CachedBlackboardComponent->SetValueAsObject("TargetActor", Actor);
            CachedBlackboardComponent->SetValueAsBool("bHasTarget", true);
            CachedBlackboardComponent->SetValueAsVector("LastKnownPosition", Stimulus.StimulusLocation);
        }

        // 시선 고정
        SetFocus(Actor, EAIFocusPriority::Gameplay);
    }
}

// MaxAge 경과 시 호출되는 안전장치 함수
void ASFEnemyController::OnTargetPerceptionForgotten(AActor* Actor)
{
    if (!Actor) return;

    if (!TargetTag.IsNone() && !Actor->ActorHasTag(TargetTag))
        return;

    // 감지된 플레이어가 여전히 있는지 확인
    TArray<AActor*> PerceivedActors;
    if (AIPerception)
    {
        AIPerception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
    }

    int32 PlayerCount = 0;
    for (AActor* PerceivedActor : PerceivedActors)
    {
        if (!TargetTag.IsNone() && PerceivedActor->ActorHasTag(TargetTag))
        {
            PlayerCount++;
        }
    }

    // 모든 타겟을 잃었을 때 처리
    if (PlayerCount == 0)
    {
        TargetActor = nullptr;
        ClearFocus(EAIFocusPriority::Gameplay);

        if (CachedBlackboardComponent)
        {
            CachedBlackboardComponent->ClearValue("TargetActor");
            CachedBlackboardComponent->SetValueAsBool("bHasTarget", false);
        }
    }
}




void ASFEnemyController::DrawDebugPerception()
{
#if !UE_BUILD_SHIPPING
    const bool bShowDebug = CVarShowAIDebug.GetValueOnGameThread();
    if (!bShowDebug) return;

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

    // 디버그 드로잉 (간소화됨)
    
    // 타겟 라인 그리기
    if (TargetActor)
    {
        DrawDebugLine(
            GetWorld(),
            ControlledPawn->GetActorLocation(),
            TargetActor->GetActorLocation(),
            FColor::Red,
            false, -1.0f, 0, 2.0f
        );
    }
#endif
}