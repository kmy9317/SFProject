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
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/Enemy/Component/SFStateReactionComponent.h"
#include "Navigation/CrowdFollowingComponent.h"

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

    bIsInCombat = false;
    TargetActor = nullptr;

    if (UCrowdFollowingComponent* CrowdComp = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
    {
        // 1. 회피 품질 설정 (높을수록 좋음, 성능 고려하여 Good~High 추천)
        CrowdComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Good);

        // 2. 충돌 감지 범위 (이 범위 내의 다른 AI를 피함)
        // 캐릭터 캡슐 크기에 따라 조절 (보통 500~700 적당)
        CrowdComp->SetCrowdCollisionQueryRange(600.f);

        // 3. 그룹 설정 (기본값 유지해도 무방하지만 명시적으로 설정)
        CrowdComp->SetAvoidanceGroup(1);
        CrowdComp->SetGroupsToAvoid(1);
        
        // 4. 서로 너무 강하게 밀치지 않도록 분리 가중치 조절
        //CrowdComp->SetCrowdSeparationWeight(50.f); 
    }
    
    // CombatComponent 생성
    CombatComponent = ObjectInitializer.CreateDefaultSubobject<USFEnemyCombatComponent>(this, TEXT("CombatComponent")); 
}

// 네트워크 복제 함수 <- 제거
void ASFEnemyController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASFEnemyController, bIsInCombat);
    DOREPLIFETIME(ASFEnemyController, bHasGuardSlot);
}

// 컨트롤러 초기화 통합 함수 (StateMachine, Combat, StateReaction 모두 처리)
void ASFEnemyController::InitializeController()
{
    if (HasAuthority())
    {
        if (APawn* InPawn = GetPawn())
        {
            // 1. StateMachine 바인딩
            BindingStateMachine(InPawn);    
        
            // 2. StateReactionComponent 바인딩 (CC기/사망 시 BT 제어를 위해 필수 [복구됨])
            USFStateReactionComponent* StateReactionComponent = USFStateReactionComponent::FindStateReactionComponent(InPawn);
            if (StateReactionComponent)
            {
                //  IsAlreadyBound를 사용하여 안전하게 바인딩 (중복 방지)
                if (!StateReactionComponent->OnStateStart.IsAlreadyBound(this, &ThisClass::ReceiveStateStart))
                {
                    StateReactionComponent->OnStateStart.AddDynamic(this, &ThisClass::ReceiveStateStart);
                }

                if (!StateReactionComponent->OnStateEnd.IsAlreadyBound(this, &ThisClass::ReceiveStateEnd))
                {
                    StateReactionComponent->OnStateEnd.AddDynamic(this, &ThisClass::ReceiveStateEnd);
                }
            }
        }
    
        // 3. CombatComponent 초기화 (거리 계산, ASC 캐싱 필수)
        if (CombatComponent)
        {
            CombatComponent->InitializeCombatComponent();
        }

        
        UE_LOG(LogTemp, Log, TEXT("[SFEnemyAI] InitializeController 완료"));
    }
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

    if (AIPerception)
    {
        AIPerception->OnTargetPerceptionUpdated.AddDynamic(
            this, &ASFEnemyController::OnTargetPerceptionUpdated
        );
        
        AIPerception->OnTargetPerceptionForgotten.AddDynamic(
            this, &ASFEnemyController::OnTargetPerceptionForgotten
        );

        UE_LOG(LogTemp, Log, TEXT("[SFEnemyAI] Perception 이벤트 바인딩 완료"));
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

    UE_LOG(LogTemp, Log, TEXT("[SFEnemyAI] 제어한 Pawn: %s"), *InPawn->GetName());

    // 스폰 위치 저장
    SpawnLocation = InPawn->GetActorLocation();

    // [수정] 통합 초기화 함수 호출로 변경 (기존 BindingStateMachine 대신 사용)
    InitializeController();
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

#pragma region BT

// [복구] 상태 이상(CC) 발생 시 BT 일시정지
void ASFEnemyController::ReceiveStateStart(FGameplayTag StateTag)
{
    if (StateTag == SFGameplayTags::Character_State_Stunned ||
        StateTag == SFGameplayTags::Character_State_Groggy ||
        StateTag == SFGameplayTags::Character_State_Parried||
        StateTag == SFGameplayTags::Character_State_Knockback ||
        StateTag == SFGameplayTags::Character_State_Knockdown ||
        StateTag == SFGameplayTags::Character_State_Dead)
    {
        if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent))
        {
            FString Reason = FString::Printf(TEXT("State Start: %s"), *StateTag.ToString());
            BTComp->PauseLogic(Reason); // BT 일시정지
            StopMovement(); // 이동도 즉시 정지
        }
    }
}

// [복구] 상태 이상 해제 시 BT 재개
void ASFEnemyController::ReceiveStateEnd(FGameplayTag StateTag)
{
    if (StateTag == SFGameplayTags::Character_State_Stunned ||
        StateTag == SFGameplayTags::Character_State_Groggy ||
        StateTag == SFGameplayTags::Character_State_Parried ||
        StateTag == SFGameplayTags::Character_State_Knockback ||
        StateTag == SFGameplayTags::Character_State_Knockdown ||
        StateTag == SFGameplayTags::Character_State_Dead)
    {
        if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent))
        {
            FString Reason = FString::Printf(TEXT("State End: %s"), *StateTag.ToString());
            BTComp->ResumeLogic(Reason); // BT 재개
        }
    }
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
    if (!InPawn) return;
    
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
    if (!ControlledPawn) return;

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
    }

    bIsInCombat = false;
    return bSuccess;
}
#pragma endregion 

void ASFEnemyController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor) return;

    if (!Stimulus.Type.IsValid() || Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>())
        return;

    if (!TargetTag.IsNone() && !Actor->ActorHasTag(TargetTag))
        return;

    //  CombatComponent에게 시야 감지 결과 전달 (거리 계산 및 태그 부여의 핵심 연결고리)
    if (CombatComponent)
    {
        CombatComponent->HandleTargetPerceptionUpdated(Actor, Stimulus.WasSuccessfullySensed());
    }

    // 감지 성공
    if (Stimulus.WasSuccessfullySensed())
    {
        TargetActor = Actor;

        if (!bIsInCombat)
        {
            bIsInCombat = true;
            SightConfig->PeripheralVisionAngleDegrees = 180.f;
            AIPerception->ConfigureSense(*SightConfig);
            UE_LOG(LogTemp, Warning, TEXT("[SFEnemyAI] 상태: Idle → Combat (타겟: %s)"), *Actor->GetName());
        }

        SetFocus(Actor, EAIFocusPriority::Gameplay);

        if (CachedBlackboardComponent)
        {
            CachedBlackboardComponent->SetValueAsObject("TargetActor", Actor);
            CachedBlackboardComponent->SetValueAsBool("bHasTarget", true);
            CachedBlackboardComponent->SetValueAsBool("bIsInCombat", true);
            CachedBlackboardComponent->SetValueAsVector("LastKnownPosition", Stimulus.StimulusLocation);
        }
    }
    // 감지 상실
    else
    {
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

        if (PlayerCount == 0 && bIsInCombat)
        {
            bIsInCombat = false;
            TargetActor = nullptr;

            SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
            AIPerception->ConfigureSense(*SightConfig);

            UE_LOG(LogTemp, Warning, TEXT("[SFEnemyAI] 상태: Combat → Idle"));

            ClearFocus(EAIFocusPriority::Gameplay);

            if (CachedBlackboardComponent)
            {
                CachedBlackboardComponent->ClearValue("TargetActor");
                CachedBlackboardComponent->SetValueAsBool("bHasTarget", false);
                CachedBlackboardComponent->SetValueAsBool("bIsInCombat", false);
            }
        }
    }
}

void ASFEnemyController::OnTargetPerceptionForgotten(AActor* Actor)
{
    if (!Actor) return;

    if (!TargetTag.IsNone() && !Actor->ActorHasTag(TargetTag))
        return;

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

    if (PlayerCount == 0 && bIsInCombat)
    {
        bIsInCombat = false;
        TargetActor = nullptr;

        if (SightConfig)
        {
            SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
            AIPerception->ConfigureSense(*SightConfig);
            UE_LOG(LogTemp, Warning, TEXT("[SFEnemyAI] 상태 변경 (Forgotten): Combat → Idle"));
        }

        ClearFocus(EAIFocusPriority::Gameplay);

        if (CachedBlackboardComponent)
        {
            CachedBlackboardComponent->ClearValue("TargetActor");
            CachedBlackboardComponent->SetValueAsBool("bHasTarget", false);
            CachedBlackboardComponent->SetValueAsBool("bIsInCombat", false);
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

        // 디버그 정보는 CombatComponent가 정확히 가지고 있으므로 여기서 그리는 건 참고용입니다.
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