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
#include "GameFramework/Character.h"
#include "Team/SFTeamTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
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
        
        if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InPawn))
        {
            RegisterCCTagEvents(ASC);
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
    //
    // APawn* MyPawn = GetPawn();
    // if (!MyPawn) return;
    //
    // // 디버그 화살표: Actor forward vs Control forward
    // DrawDebugDirectionalArrow(GetWorld(),
    //     MyPawn->GetActorLocation(),
    //     MyPawn->GetActorLocation() + MyPawn->GetActorForwardVector() * 200.f,
    //     100.f, FColor::Red, false, -1.f, 0, 5.f);
    //
    // FRotator ControlRot = GetControlRotation();
    // FVector ControlDir = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::X);
    //
    // DrawDebugDirectionalArrow(GetWorld(),
    //     MyPawn->GetActorLocation() + FVector(0, 0, 10),
    //     MyPawn->GetActorLocation() + ControlDir * 200.f + FVector(0, 0, 10),
    //     100.f, FColor::Green, false, -1.f, 0, 5.f);
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

}

void ASFBaseAIController::OnUnPossess()
{
    UnBindingStateMachine();
    Super::OnUnPossess();
}

void ASFBaseAIController::RegisterCCTagEvents(UAbilitySystemComponent* ASC)
{
    if (!ASC) return;

    // PhaseIntro
    ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_PhaseIntro, EGameplayTagEventType::NewOrRemoved)
        .AddUObject(this, &ThisClass::OnCCTagChanged);

    // Stunned
    ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Stunned, EGameplayTagEventType::NewOrRemoved)
        .AddUObject(this, &ThisClass::OnCCTagChanged);

    // Groggy
    ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Groggy, EGameplayTagEventType::NewOrRemoved)
        .AddUObject(this, &ThisClass::OnCCTagChanged);

    // Dead
    ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Dead, EGameplayTagEventType::NewOrRemoved)
        .AddUObject(this, &ThisClass::OnCCTagChanged);
}

void ASFBaseAIController::OnCCTagChanged(const FGameplayTag Tag, int32 NewCount)
{
    if (!HasAuthority()) return;

    if (NewCount > 0)
    {
        OnCCStart(Tag);
    }
    else
    {
        OnCCEnd(Tag);
    }
}

void ASFBaseAIController::OnCCStart(FGameplayTag StateTag)
{
    if (!HasAuthority()) return;

    if (StateTag == SFGameplayTags::Character_State_Stunned ||
        StateTag == SFGameplayTags::Character_State_Dead ||
        StateTag == SFGameplayTags::Character_State_Groggy ||
        StateTag == SFGameplayTags::Character_State_PhaseIntro||
        StateTag == SFGameplayTags::Character_State_Hit)
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

void ASFBaseAIController::OnCCEnd(FGameplayTag StateTag)
{
    if (!HasAuthority() || StateTag == SFGameplayTags::Character_State_Dead) return;

    SetActorTickEnabled(true);
    if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent))
    {
        BTComp->ResumeLogic(TEXT("CC End"));
    }
    
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
    
    Super::UpdateControlRotation(DeltaTime, false);
    
    if (CombatComponent && CombatComponent->GetCurrentTarget())
    {
        
        if (GetFocusActor() != CombatComponent->GetCurrentTarget())
        {
            SetFocus(CombatComponent->GetCurrentTarget());
        }
    }
    else
    {
        if (GetFocusActor())
        {
            ClearFocus(EAIFocusPriority::Gameplay);
        }
    }
    
    if (ShouldRotateActorByController())
    {
        RotateActorTowardsController(DeltaTime); 
    }
}

void ASFBaseAIController::SetRotationMode(EAIRotationMode NewMode)
{

    ACharacter* Char = Cast<ACharacter>(GetPawn());
    if (!Char) return;

    UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement();
    if (!MoveComp) return;

    CurrentRotationMode = NewMode;

    switch (NewMode)
    {
    case EAIRotationMode::MovementDirection:
        MoveComp->bOrientRotationToMovement = true; 
        MoveComp->bUseControllerDesiredRotation = false; 
        break;

    case EAIRotationMode::ControllerYaw:
        MoveComp->bOrientRotationToMovement = false; 
        MoveComp->bUseControllerDesiredRotation = false; 
        break;

    case EAIRotationMode::None: 
        MoveComp->bOrientRotationToMovement = false;
        MoveComp->bUseControllerDesiredRotation = false;
        MoveComp->RotationRate = FRotator::ZeroRotator;
        break;
    }
}
