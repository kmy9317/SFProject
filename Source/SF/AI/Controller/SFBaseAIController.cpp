#include "SFBaseAIController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AI/StateMachine/SFStateMachine.h"
#include "Animation/Enemy/SFEnemyAnimInstance.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Net/UnrealNetwork.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Character/Enemy/Component/SFStateReactionComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "Team/SFTeamTypes.h"
#include "Navigation/CrowdFollowingComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFBaseAIController)

ASFBaseAIController::ASFBaseAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bIsInCombat = false;
    TargetActor = nullptr;

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void ASFBaseAIController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASFBaseAIController, bIsInCombat);
    DOREPLIFETIME(ASFBaseAIController, TeamId);
}

void ASFBaseAIController::InitializeAIController()
{
    if (HasAuthority())
    {
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

        SetGenericTeamId((FGenericTeamId(SFTeamID::Enemy)));
    }
    
    if (USFPawnExtensionComponent* PawnExtensionComp = USFPawnExtensionComponent::FindPawnExtensionComponent(GetPawn()))
    {
        if (const USFEnemyData* EnemyData = PawnExtensionComp->GetPawnData<USFEnemyData>())
        {
            BehaviorTreeContainer = EnemyData->BehaviourContainer;
            
        }
    }
}

USFEnemyCombatComponent* ASFBaseAIController::GetCombatComponent() const
{
    if (IsValid(CombatComponent))
    {
        return CombatComponent;
    }
    return nullptr;
}

void ASFBaseAIController::PreInitializeComponents()
{
    Super::PreInitializeComponents();
    UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
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
    
    if (!InPawn)
        return;

    USFPawnExtensionComponent* PawnExtensionComp = 
        USFPawnExtensionComponent::FindPawnExtensionComponent(InPawn);
    
    if (!PawnExtensionComp)
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] PawnExtensionComp is NULL!"), *GetName());
        return;
    }

    const USFEnemyData* EnemyData = PawnExtensionComp->GetPawnData<USFEnemyData>();
    if (!EnemyData)
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] EnemyData is NULL!"), *GetName());
        return;
    }

    BehaviorTreeContainer = EnemyData->BehaviourContainer;
    
    if (BehaviorTreeContainer.GetBehaviourTree(EnemyData->DefaultBehaviourTag))
    {
            
        SetBehaviorTree(BehaviorTreeContainer.GetBehaviourTree(EnemyData->DefaultBehaviourTag));
        
    }
    
  
    
}

void ASFBaseAIController::OnUnPossess()
{
    UnBindingStateMachine();
    Super::OnUnPossess();
}

#pragma region BehaviorTree

void ASFBaseAIController::ReceiveStateStart(FGameplayTag StateTag)
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

      
        ClearFocus(EAIFocusPriority::Gameplay);

        TargetActor = nullptr;

        if (CachedBlackboardComponent)
        {
            CachedBlackboardComponent->ClearValue("TargetActor");
            CachedBlackboardComponent->SetValueAsBool("bHasTarget",false);
        }

        // CC 상태에서는 회전 중지
        SetRotationMode(EAIRotationMode::None);
    }
}

void ASFBaseAIController::ReceiveStateEnd(FGameplayTag StateTag)
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

        // CC 해제 시 전투 중이면 ControllerYaw, 아니면 MovementDirection으로 복귀
        if (bIsInCombat)
        {
            SetRotationMode(EAIRotationMode::ControllerYaw);
        }
        else
        {
            SetRotationMode(EAIRotationMode::MovementDirection);
        }
    }
}

void ASFBaseAIController::SetBehaviorTree(UBehaviorTree* NewBehaviorTree)
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

void ASFBaseAIController::BindingStateMachine(const APawn* InPawn)
{
    if (!InPawn) return;
    
    USFStateMachine* StateMachine = USFStateMachine::FindStateMachineComponent(InPawn);
    if (StateMachine)
    {
        StateMachine->OnChangeTreeDelegate.AddUObject(this, &ThisClass::ChangeBehaviorTree);
        StateMachine->OnStopTreeDelegate.AddUObject(this, &ThisClass::StopBehaviorTree);
    }
}

void ASFBaseAIController::UnBindingStateMachine()
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

void ASFBaseAIController::StopBehaviorTree()
{
    if (CachedBehaviorTreeComponent)
        CachedBehaviorTreeComponent->StopTree();
}

void ASFBaseAIController::ChangeBehaviorTree(FGameplayTag GameplayTag)
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

bool ASFBaseAIController::RunBehaviorTree(UBehaviorTree* BehaviorTree)
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

void ASFBaseAIController::SetGenericTeamId(const FGenericTeamId& InTeamID)
{
    if (HasAuthority())
    {
        TeamId = InTeamID;
    }
}

FGenericTeamId ASFBaseAIController::GetGenericTeamId() const
{
    return TeamId;
}



void ASFBaseAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
    APawn* MyPawn = GetPawn();
    if (!MyPawn || CurrentRotationMode == EAIRotationMode::None)
        return;

    // TurnInPlace 모드: 점진적 회전 (AnimInstance가 RootYawOffset으로 메시 회전)
    if (CurrentRotationMode == EAIRotationMode::TurnInPlace)
    {
        UpdateControlRotationTowardsFocus(DeltaTime);
        return;
    }

    // MovementDirection 모드: CharacterMovement가 처리
    if (CurrentRotationMode == EAIRotationMode::MovementDirection)
    {
        return;
    }

    // ControllerYaw 모드: 점진적 회전 (AnimInstance가 RootYawOffset 누적)
    // ⚠️ Super::UpdateControlRotation()은 즉시 회전하므로 사용하지 않음!
    // 대신 점진적으로 ControlRotation을 Target 방향으로 회전
    if (CurrentRotationMode == EAIRotationMode::ControllerYaw)
    {
        UpdateControlRotationTowardsFocus(DeltaTime);
        return;
    }
}



void ASFBaseAIController::UpdateControlRotationTowardsFocus(float DeltaTime)
{
    APawn* MyPawn = GetPawn();
    if (!MyPawn) return;

    const FVector FocalPoint = GetFocalPoint();
    if (!FAISystem::IsValidLocation(FocalPoint))
        return;

    FVector ToTarget = FocalPoint - MyPawn->GetActorLocation();
    ToTarget.Z = 0.f;

    if (ToTarget.IsNearlyZero()) return;

    FRotator TargetRot = ToTarget.Rotation();
    FRotator CurrentRot = GetControlRotation();
    float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentRot.Yaw, TargetRot.Yaw);

    const float RotationDeadZone = 1.0f;
    if (FMath::Abs(DeltaYaw) < RotationDeadZone)
        return;

    FRotator NewRotation = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, ControlRotationInterpSpeed);
    SetControlRotation(FRotator(0.f, NewRotation.Yaw, 0.f));
}

void ASFBaseAIController::SetRotationMode(EAIRotationMode NewMode)
{
    if (CurrentRotationMode == NewMode)
        return;

    ACharacter* Char = Cast<ACharacter>(GetPawn());
    if (!Char) return;

    UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement();
    if (!MoveComp) return;

    USFEnemyAnimInstance* AnimInstance = nullptr;
    if (USkeletalMeshComponent* Mesh = Char->GetMesh())
    {
        AnimInstance = Cast<USFEnemyAnimInstance>(Mesh->GetAnimInstance());
    }

    EAIRotationMode PreviousMode = CurrentRotationMode;

    // Ability 종료 시 ControlRotation을 Pawn Yaw로 동기화
    if (PreviousMode == EAIRotationMode::None && NewMode != EAIRotationMode::None)
    {
        FRotator CurrentPawnRotation = Char->GetActorRotation();
        SetControlRotation(FRotator(0.f, CurrentPawnRotation.Yaw, 0.f));

        if (AnimInstance)
        {
            AnimInstance->ResetTurnInPlaceState();
        }

        if (TargetActor)
        {
            SetFocus(TargetActor, EAIFocusPriority::Gameplay);
        }
    }

    CurrentRotationMode = NewMode;

    switch (NewMode)
    {
    case EAIRotationMode::None:
        MoveComp->bOrientRotationToMovement = false;
        MoveComp->bUseControllerDesiredRotation = false;
        break;

    case EAIRotationMode::MovementDirection:
        MoveComp->bOrientRotationToMovement = true;
        MoveComp->bUseControllerDesiredRotation = false;
        MoveComp->RotationRate = FRotator(0.f, 120.f, 0.f);

        if (AnimInstance)
        {
            AnimInstance->ResetTurnInPlaceState();
        }
        break;

    case EAIRotationMode::TurnInPlace:
        MoveComp->bOrientRotationToMovement = false;
        MoveComp->bUseControllerDesiredRotation = false;
        break;

    case EAIRotationMode::ControllerYaw:
        MoveComp->bOrientRotationToMovement = false;
        MoveComp->bUseControllerDesiredRotation = false;

        if (TargetActor && !GetFocusActor())
        {
            SetFocus(TargetActor, EAIFocusPriority::Gameplay);
        }
        break;
    }
}

#pragma endregion

