// SFBaseAIController.cpp
#include "SFBaseAIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AI/StateMachine/SFStateMachine.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Net/UnrealNetwork.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Character/Enemy/Component/SFStateReactionComponent.h"
#include "GameFramework/Character.h"
#include "Team/SFTeamTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFBaseAIController)

ASFBaseAIController::ASFBaseAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bIsInCombat = false;
    TargetActor = nullptr;

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    
    CurrentRotationMode = EAIRotationMode::MovementDirection;
    RotationInterpSpeed = 5.f;
}

void ASFBaseAIController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ASFBaseAIController, bIsInCombat);
    DOREPLIFETIME(ASFBaseAIController, TeamId);
}

void ASFBaseAIController::InitializeAIController()
{
    if (!HasAuthority()) return;

    if (APawn* InPawn = GetPawn())
    {
        BindingStateMachine(InPawn);

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

    if (CombatComponent)
    {
        CombatComponent->InitializeCombatComponent();
    }

    SetGenericTeamId(FGenericTeamId(SFTeamID::Enemy));

    if (USFPawnExtensionComponent* PawnExtensionComp = USFPawnExtensionComponent::FindPawnExtensionComponent(GetPawn()))
    {
        if (const USFEnemyData* EnemyData = PawnExtensionComp->GetPawnData<USFEnemyData>())
        {
            BehaviorTreeContainer = EnemyData->BehaviourContainer;
        }
    }
}

void ASFBaseAIController::PreInitializeComponents()
{
    Super::PreInitializeComponents();
    UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

USFCombatComponentBase* ASFBaseAIController::GetCombatComponent() const
{
    if (IsValid(CombatComponent))
    {
        return CombatComponent;
    }
    return nullptr;
}

void ASFBaseAIController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    APawn* MyPawn = GetPawn();
    if (!MyPawn) return;

    // 디버그 화살표: Actor forward vs Control forward
    DrawDebugDirectionalArrow(GetWorld(),
        MyPawn->GetActorLocation(),
        MyPawn->GetActorLocation() + MyPawn->GetActorForwardVector() * 200.f,
        100.f, FColor::Red, false, -1.f, 0, 5.f);

    FRotator ControlRot = GetControlRotation();
    FVector ControlDir = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::X);

    DrawDebugDirectionalArrow(GetWorld(),
        MyPawn->GetActorLocation() + FVector(0, 0, 10),
        MyPawn->GetActorLocation() + ControlDir * 200.f + FVector(0, 0, 10),
        100.f, FColor::Green, false, -1.f, 0, 5.f);
}

void ASFBaseAIController::BeginPlay()
{
    UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);
    Super::BeginPlay();
}

void ASFBaseAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
    Super::EndPlay(EndPlayReason);
}

void ASFBaseAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (!InPawn) return;

    USFPawnExtensionComponent* PawnExtensionComp = USFPawnExtensionComponent::FindPawnExtensionComponent(InPawn);
    if (!PawnExtensionComp) return;

    const USFEnemyData* EnemyData = PawnExtensionComp->GetPawnData<USFEnemyData>();
    if (!EnemyData) return;

    BehaviorTreeContainer = EnemyData->BehaviourContainer;

    UBehaviorTree* BT = BehaviorTreeContainer.GetBehaviourTree(EnemyData->DefaultBehaviourTag);
    if (BT) SetBehaviorTree(BT);
}

void ASFBaseAIController::OnUnPossess()
{
    UnBindingStateMachine();
    Super::OnUnPossess();
}

void ASFBaseAIController::ReceiveStateStart(FGameplayTag StateTag)
{
    if (!HasAuthority()) return;

    if (StateTag == SFGameplayTags::Character_State_Stunned ||
        StateTag == SFGameplayTags::Character_State_Dead)
    {
        SetActorTickEnabled(false);
        if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent))
        {
            BTComp->PauseLogic(TEXT("CC Start"));
            StopMovement();
        }

        if (ACharacter* Char = Cast<ACharacter>(GetPawn()))
        {
            if (UCharacterMovementComponent* MC = Char->GetCharacterMovement())
            {
                MC->bOrientRotationToMovement = false;
                MC->bUseControllerDesiredRotation = false;
                MC->RotationRate = FRotator::ZeroRotator;
            }
        }
    }
}

void ASFBaseAIController::ReceiveStateEnd(FGameplayTag StateTag)
{
    if (!HasAuthority() || StateTag == SFGameplayTags::Character_State_Dead) return;

    SetActorTickEnabled(true);
    if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent))
    {
        BTComp->ResumeLogic(TEXT("CC End"));
    }

    // CC 해제 후 현재 전투 상태에 맞는 회전 모드로 복원
    // 하위 클래스(Enemy, Dragon)가 각자의 방식으로 처리하므로
    // Base에서는 기본 MovementDirection으로만 설정
    SetRotationMode(EAIRotationMode::MovementDirection);
}

void ASFBaseAIController::SetBehaviorTree(UBehaviorTree* NewBehaviorTree)
{
    if (!NewBehaviorTree) return;
    if (CachedBehaviorTreeComponent && CachedBehaviorTreeComponent->GetCurrentTree() == NewBehaviorTree) return;
    RunBehaviorTree(NewBehaviorTree);
}

void ASFBaseAIController::BindingStateMachine(const APawn* InPawn)
{
    if (!InPawn) return;

    USFStateMachine* StateMachine = USFStateMachine::FindStateMachineComponent(InPawn);
    if (!StateMachine) return;

    StateMachine->OnChangeTreeDelegate.AddUObject(this, &ThisClass::ChangeBehaviorTree);
    StateMachine->OnStopTreeDelegate.AddUObject(this, &ThisClass::StopBehaviorTree);
}

void ASFBaseAIController::UnBindingStateMachine()
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

    USFStateMachine* StateMachine = USFStateMachine::FindStateMachineComponent(ControlledPawn);
    if (!StateMachine) return;

    StateMachine->OnChangeTreeDelegate.RemoveAll(this);
    StateMachine->OnStopTreeDelegate.RemoveAll(this);
}

void ASFBaseAIController::StopBehaviorTree()
{
    if (CachedBehaviorTreeComponent) CachedBehaviorTreeComponent->StopTree();
}

void ASFBaseAIController::ChangeBehaviorTree(FGameplayTag GameplayTag)
{
    if (!GameplayTag.IsValid()) return;
    UBehaviorTree* NewTree = BehaviorTreeContainer.GetBehaviourTree(GameplayTag);
    if (NewTree) SetBehaviorTree(NewTree);
}

bool ASFBaseAIController::RunBehaviorTree(UBehaviorTree* BehaviorTree)
{
    if (!BehaviorTree) return false;
    const bool bSuccess = Super::RunBehaviorTree(BehaviorTree);
    if (bSuccess)
    {
        CachedBehaviorTreeComponent = Cast<UBehaviorTreeComponent>(GetBrainComponent());
        CachedBlackboardComponent = GetBlackboardComponent();
    }
    bIsInCombat = false;
    return bSuccess;
}

void ASFBaseAIController::SetGenericTeamId(const FGenericTeamId& InTeamID)
{
    if (HasAuthority()) TeamId = InTeamID;
}

FGenericTeamId ASFBaseAIController::GetGenericTeamId() const
{
    return TeamId;
}

bool ASFBaseAIController::ShouldRotateActorByController() const
{
    return true;
}

void ASFBaseAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
    APawn* MyPawn = GetPawn();
    if (!MyPawn) return;
    
    if (IsTurningInPlace()) return;
    
    bool bIsUsingAbility = false;
    if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MyPawn))
    {
        if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_UsingAbility))
        {
            bIsUsingAbility = true;
        }
    }
    // Controller 회전만 갱신 
    Super::UpdateControlRotation(DeltaTime, false);
    
    if (bIsUsingAbility)
    {
        return;
    }
    
    if (CombatComponent && CombatComponent->GetCurrentTarget())
    {
        FVector ToTarget = CombatComponent->GetCurrentTarget()->GetActorLocation() - MyPawn->GetActorLocation();
        ToTarget.Z = 0.f;
        if (!ToTarget.IsNearlyZero())
        {
            SetControlRotation(ToTarget.Rotation());
        }
    }
    
    if (!ShouldRotateActorByController()) return;

    // ControllerYaw 모드일 때만 Base에서 Actor 회전 처리
    if (CurrentRotationMode == EAIRotationMode::ControllerYaw)
    {
        FRotator TargetRot = GetControlRotation();
        TargetRot.Pitch = 0.f;
        TargetRot.Roll = 0.f;

        FRotator CurrentActorRot = MyPawn->GetActorRotation();
        float AngleDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentActorRot.Yaw, TargetRot.Yaw));
        float Threshold = GetTurnThreshold();
        
        if (AngleDiff < Threshold)
        {
            FRotator NewActorRot = FMath::RInterpTo(CurrentActorRot, TargetRot, DeltaTime, RotationInterpSpeed);
            MyPawn->SetActorRotation(NewActorRot);
        }
    }
}

void ASFBaseAIController::SetRotationMode(EAIRotationMode NewMode)
{
    if (CurrentRotationMode == NewMode) return;

    ACharacter* Char = Cast<ACharacter>(GetPawn());
    if (!Char) return;

    UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement();
    if (!MoveComp) return;

    CurrentRotationMode = NewMode;

    switch (NewMode)
    {
    case EAIRotationMode::None:
        MoveComp->bOrientRotationToMovement = false;
        MoveComp->bUseControllerDesiredRotation = false;
        MoveComp->RotationRate = FRotator::ZeroRotator;
        break;

    case EAIRotationMode::MovementDirection:
        MoveComp->bOrientRotationToMovement = true;
        MoveComp->bUseControllerDesiredRotation = false;
        MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
        break;

    case EAIRotationMode::ControllerYaw:
        MoveComp->bOrientRotationToMovement = false;
        MoveComp->bUseControllerDesiredRotation = false;
        MoveComp->RotationRate = FRotator::ZeroRotator;
        break;
    }
}