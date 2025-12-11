// Fill out your copyright notice in the Description page of Project Settings.

#include "SFEnemyController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
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
#include "Team/SFTeamTypes.h"

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

void ASFEnemyController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASFEnemyController, bIsInCombat);
    DOREPLIFETIME(ASFEnemyController, bHasGuardSlot);
    DOREPLIFETIME(ASFEnemyController, TeamId);
}

// 컨트롤러 초기화 통합 함수 (StateMachine, Combat, StateReaction 모두 처리)
void ASFEnemyController::InitializeController()
{
    if (HasAuthority())
    {
        //AIPerception 바인딩 
        if (AIPerception)
        {
            AIPerception->OnTargetPerceptionUpdated.AddDynamic(
                this, &ASFEnemyController::OnTargetPerceptionUpdated
            );
        
            AIPerception->OnTargetPerceptionForgotten.AddDynamic(
                this, &ASFEnemyController::OnTargetPerceptionForgotten
            );
        }
        
        if (APawn* InPawn = GetPawn())
        {
            //  StateMachine 바인딩
            BindingStateMachine(InPawn);    
        
            // 2. StateReactionComponent 바인딩 (CC기/사망 시 BT 제어를 위해 필수)
            USFStateReactionComponent* StateReactionComponent = USFStateReactionComponent::FindStateReactionComponent(InPawn);
            if (StateReactionComponent)
            {
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

        SetGenericTeamId((FGenericTeamId(SFTeamID::Enemy)));
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

    // 스폰 위치 저장
    SpawnLocation = InPawn->GetActorLocation();
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
    if (!HasAuthority())
    {
        return;
    }
    if (StateTag == SFGameplayTags::Character_State_Stunned ||
        StateTag == SFGameplayTags::Character_State_Groggy ||
        StateTag == SFGameplayTags::Character_State_Parried||
        StateTag == SFGameplayTags::Character_State_Knockback ||
        StateTag == SFGameplayTags::Character_State_Knockdown ||
        StateTag == SFGameplayTags::Character_State_Dead)
    {
        SetActorTickEnabled(false);
        if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent))
        {
            FString Reason = FString::Printf(TEXT("State Start: %s"), *StateTag.ToString());
            BTComp->PauseLogic(Reason); // BT 일시정지
            StopMovement(); // 이동도 즉시 정지
        }
        
        // [중요 예외] BT가 멈추면 서비스도 멈추므로, 여기서는 직접 풀어줘야 안전함!
        ClearFocus(EAIFocusPriority::Gameplay);
        
        TargetActor = nullptr;

        if (CachedBlackboardComponent)
        {
            CachedBlackboardComponent->ClearValue("TargetActor");
            CachedBlackboardComponent->SetValueAsBool("bHasTarget",false);
        }
    }
}

// [복구] 상태 이상 해제 시 BT 재개
void ASFEnemyController::ReceiveStateEnd(FGameplayTag StateTag)
{
    // Death 상태는 재개하지 않음
    if (!HasAuthority())
    {
        return;
    }
    if (StateTag == SFGameplayTags::Character_State_Dead)
    {
        return;
    }
    
    if (StateTag == SFGameplayTags::Character_State_Stunned ||
        StateTag == SFGameplayTags::Character_State_Groggy ||
        StateTag == SFGameplayTags::Character_State_Parried ||
        StateTag == SFGameplayTags::Character_State_Knockback ||
        StateTag == SFGameplayTags::Character_State_Knockdown)
    {
        APawn* ControlledPawn = GetPawn();
        if (!ControlledPawn)
        {
            return;
        }
        
        UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ControlledPawn);
        if (!ASC)
        {
            return;
        }
        
        FGameplayTagContainer CCStateTags;
        CCStateTags.AddTag(SFGameplayTags::Character_State_Stunned);
        CCStateTags.AddTag(SFGameplayTags::Character_State_Groggy);
        CCStateTags.AddTag(SFGameplayTags::Character_State_Parried);
        CCStateTags.AddTag(SFGameplayTags::Character_State_Knockback);
        CCStateTags.AddTag(SFGameplayTags::Character_State_Knockdown);
        CCStateTags.AddTag(SFGameplayTags::Character_State_Dead);
        
        if (ASC->HasAnyMatchingGameplayTags(CCStateTags))
        {
            return;
        }
        
        SetActorTickEnabled(true);
        
        if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent))
        {
            FString Reason = FString::Printf(TEXT("State End"), *StateTag.ToString());
            BTComp->ResumeLogic(Reason);
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
        SetBehaviorTree(NewTree);
    }
}

bool ASFEnemyController::RunBehaviorTree(UBehaviorTree* BehaviorTree)
{
    if (!BehaviorTree)
    {
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

    //  CombatComponent에게 시야 감지 결과 전달
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
        }

        // [삭제됨] 직접 SetFocus 호출 -> 이제 BTService (SFBTS_UpdateFocus)가 담당함
        // SetFocus(Actor, EAIFocusPriority::Gameplay); 

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

            // [삭제됨] 직접 ClearFocus 호출 -> BTService가 담당
            // ClearFocus(EAIFocusPriority::Gameplay);

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
        }

        // [삭제됨] 직접 ClearFocus 호출 -> BTService가 담당
        // ClearFocus(EAIFocusPriority::Gameplay);

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

void ASFEnemyController::SetGenericTeamId(const FGenericTeamId& InTeamID)
{
    if (HasAuthority())
    {
        TeamId = InTeamID;
    }
}

FGenericTeamId ASFEnemyController::GetGenericTeamId() const
{
    return TeamId;
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

    // [삭제됨] 직접 SetFocus 호출 -> BTService가 담당
    // SetFocus(NewTarget, EAIFocusPriority::Gameplay);
    
    // 5. CombatComponent에도 알림 (거리 계산, 공격 가능 여부 판단 등을 위해 필수)
    if (CombatComponent)
    {
        // 강제로 감지된 것으로 처리하여 내부 상태 갱신
        CombatComponent->HandleTargetPerceptionUpdated(NewTarget, true);
    }
}