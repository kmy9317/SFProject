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
#include "Character/SFCharacterGameplayTags.h"
#include "Character/Enemy/Component/SFStateReactionComponent.h"

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

    // bIsInCombat 복제 (서버 → 클라이언트)
    DOREPLIFETIME(ASFEnemyController, bIsInCombat);
    DOREPLIFETIME(ASFEnemyController, bHasGuardSlot);

}

void ASFEnemyController::InitializeController()
{
    if (HasAuthority())
    {
        if (APawn* InPawn = GetPawn())
        {
            // 상태 머신 이벤트 바인딩
            BindingStateMachine(InPawn);    
        }
    
        if (CombatComponent)
        {
            CombatComponent->InitializeCombatComponent();
        }
    
        USFStateReactionComponent* StateReactionComponent = USFStateReactionComponent::FindStateReactionComponent(GetPawn());
        if (StateReactionComponent)
        {
            StateReactionComponent->OnStateStart.AddDynamic(this, &ThisClass::ReceiveStateStart);
            StateReactionComponent->OnStateEnd.AddDynamic(this, &ThisClass::ReceiveStateEnd);
        }
        
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

    // 디버그 - 현재 범위 출력
    UE_LOG(LogTemp, Warning, TEXT("[SFEnemyAI] 감지 범위 → 근접: %.0f | 경계: %.0f | 시야: %.0f | 상실: %.0f"),
        MeleeRange, GuardRange, SightRadius, LoseSightRadius);
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
        UE_LOG(LogTemp, Warning, TEXT("[SFEnemyAI] OnPossess 실패: Pawn 이 nullptr 입니다"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[SFEnemyAI] 제어한 Pawn: %s"), *InPawn->GetName());

    // 스폰 위치 저장 (귀환 행동 등에 사용 가능)
    SpawnLocation = InPawn->GetActorLocation();
    UE_LOG(LogTemp, Log, TEXT("[SFEnemyAI] Spawn 위치 저장: %s"), *SpawnLocation.ToString());


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
            BTComp->PauseLogic("State Start"); // BT 일시정지
        }
    }
}

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
            BTComp->ResumeLogic("State End"); // BT 재개
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
#pragma endregion 
void ASFEnemyController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor)
        return;

    // 시야 감각만 처리
    if (!Stimulus.Type.IsValid() || Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>())
        return;

    // 특정 태그만 타겟으로 감지하려는 경우
    if (!TargetTag.IsNone() && !Actor->ActorHasTag(TargetTag))
        return;

    UE_LOG(LogTemp, Log, TEXT("[SFEnemyAI] 감지된 액터: %s (Tag: %s)"), *Actor->GetName(), *TargetTag.ToString());

    // 감지 성공 (전투 상태 진입)
    if (Stimulus.WasSuccessfullySensed())
    {
        TargetActor = Actor;

        if (!bIsInCombat)
        {
            bIsInCombat = true;

            // 전투 중에는 시야 360도
            SightConfig->PeripheralVisionAngleDegrees = 180.f;
            AIPerception->ConfigureSense(*SightConfig);

            UE_LOG(LogTemp, Warning, TEXT("[SFEnemyAI] 상태: Idle → Combat (타겟: %s) | 시야 90° → 360°"),
                *Actor->GetName());
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
    // 감지 상실 (전투 종료 체크)
    else
    {
        // 여기서는 당장 잃었다고 전투를 끄지 않고, 모든 타겟을 놓쳤는지 확인
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

            // Idle 시야 각도 복원
            SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
            AIPerception->ConfigureSense(*SightConfig);

            UE_LOG(LogTemp, Warning, TEXT("[SFEnemyAI] 상태: Combat → Idle | 시야 360° → 90°"));

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

// [추가] MaxAge 경과 시 호출되는 안전장치 함수
void ASFEnemyController::OnTargetPerceptionForgotten(AActor* Actor)
{
    if (!Actor)
        return;

    UE_LOG(LogTemp, Log, TEXT("[SFEnemyAI] OnTargetPerceptionForgotten: %s (MaxAge 만료)"), *Actor->GetName());

    // TargetTag 필터링
    if (!TargetTag.IsNone() && !Actor->ActorHasTag(TargetTag))
        return;

    // 감지된 플레이어가 여전히 있는지 확인 (이중 안전장치)
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

    // 모든 플레이어를 잃었으면 Combat → Idle 전환
    if (PlayerCount == 0 && bIsInCombat)
    {
        bIsInCombat = false;
        TargetActor = nullptr;

        // Idle 상태: 90도 전방 원뿔로 복원
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


float ASFEnemyController::GetDistanceToTarget() const
{
    if (!TargetActor || !GetPawn())
        return 0.f;

    return FVector::Dist(GetPawn()->GetActorLocation(), TargetActor->GetActorLocation());
}

bool ASFEnemyController::IsInMeleeRange() const
{
    const float Distance = GetDistanceToTarget();
    return Distance > 0.f && Distance <= MeleeRange;
}

bool ASFEnemyController::IsInGuardRange() const
{
    const float Distance = GetDistanceToTarget();
    return Distance > 0.f && Distance <= GuardRange;
}

bool ASFEnemyController::IsInTrackingRange() const
{
    const float Distance = GetDistanceToTarget();
    return Distance > 0.f && Distance <= LoseSightRadius;
}

// [제거됨] FindBestTarget 구현부 삭제


void ASFEnemyController::DrawDebugPerception()
{
#if !UE_BUILD_SHIPPING
    const bool bShowDebug = CVarShowAIDebug.GetValueOnGameThread();
    if (!bShowDebug)
        return;

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
        return;

    const FVector PawnLocation = ControlledPawn->GetActorLocation();
    const FRotator PawnRotation = ControlledPawn->GetActorRotation();
    const FVector ForwardVector = PawnRotation.Vector();
    const float LifeTime = 0.f;

    FVector EyeLocation = PawnLocation;
    if (ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
        EyeLocation.Z += ControlledCharacter->BaseEyeHeight;
    else
        EyeLocation.Z += 90.f;

    // Idle 시 상태
    if (!bIsInCombat)
    {
        DrawDebugCone(
            GetWorld(),
            EyeLocation,
            ForwardVector,
            SightRadius,
            FMath::DegreesToRadians(PeripheralVisionAngleDegrees),
            FMath::DegreesToRadians(PeripheralVisionAngleDegrees),
            16,
            FColor::Green,
            false,
            LifeTime,
            0,
            2.0f
        );

        DrawDebugSphere(
            GetWorld(),
            PawnLocation,
            SightRadius,
            16,
            FColor::Green,
            false,
            LifeTime,
            0,
            1.0f
        );

        DrawDebugString(
            GetWorld(),
            PawnLocation + FVector(0, 0, 150),
            TEXT("STATE: IDLE"),
            nullptr,
            FColor::White,
            LifeTime,
            true,
            1.5f
        );
    }
    // 전투 상태
    else
    {
        DrawDebugCone(
            GetWorld(),
            EyeLocation,
            ForwardVector,
            SightRadius,
            FMath::DegreesToRadians(180.f),
            FMath::DegreesToRadians(180.f),
            16,
            FColor::Cyan,
            false,
            LifeTime,
            0,
            1.5f
        );

        DrawDebugSphere(GetWorld(), PawnLocation, MeleeRange, 12, FColor::Red, false, LifeTime, 0, 4.0f);
        DrawDebugSphere(GetWorld(), PawnLocation, GuardRange, 16, FColor::Yellow, false, LifeTime, 0, 2.5f);
        DrawDebugSphere(GetWorld(), PawnLocation, LoseSightRadius, 16, FColor::Orange, false, LifeTime, 0, 2.0f);

        if (TargetActor)
        {
            DrawDebugLine(
                GetWorld(),
                PawnLocation + FVector(0, 0, 50),
                TargetActor->GetActorLocation() + FVector(0, 0, 50),
                FColor::Cyan,
                false,
                LifeTime,
                0,
                2.0f
            );

            const float Distance = GetDistanceToTarget();
            DrawDebugString(
                GetWorld(),
                PawnLocation + FVector(0, 0, 150),
                FString::Printf(TEXT("Distance: %.0f"), Distance),
                nullptr,
                FColor::Yellow,
                LifeTime,
                true,
                1.2f
            );

            FString StateText;
            FColor StateColor;
            if (Distance <= MeleeRange)
            {
                StateText = TEXT("STATE: MELEE");
                StateColor = FColor::Red;
            }
            else if (Distance <= GuardRange)
            {
                StateText = TEXT("STATE: GUARD");
                StateColor = FColor::Yellow;
            }
            else
            {
                StateText = TEXT("STATE: CHASE");
                StateColor = FColor::Orange;
            }

            DrawDebugString(
                GetWorld(),
                PawnLocation + FVector(0, 0, 120),
                StateText,
                nullptr,
                StateColor,
                LifeTime,
                true,
                1.5f
            );
        }
    }
#endif
}