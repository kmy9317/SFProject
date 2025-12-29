#include "SFBaseAIController.h"
#include "SFTurnInPlaceComponent.h"

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
#include "Team/SFTeamTypes.h"
#include "Navigation/CrowdFollowingComponent.h"
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

    // TurnInPlaceComponent 생성
    TurnInPlaceComponent = CreateDefaultSubobject<USFTurnInPlaceComponent>(TEXT("TurnInPlaceComponent"));
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
        
        if (USFPawnExtensionComponent* PawnExtensionComp = USFPawnExtensionComponent::FindPawnExtensionComponent(GetPawn()))
        {
            if (const USFEnemyData* EnemyData = PawnExtensionComp->GetPawnData<USFEnemyData>())
            {
                BehaviorTreeContainer = EnemyData->BehaviourContainer;
                
            }
        }


        if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn()))
        {
            ASC->RegisterGameplayTagEvent(
                SFGameplayTags::Character_State_UsingAbility,
                EGameplayTagEventType::NewOrRemoved
            ).AddUObject(this, &ThisClass::OnAbilityStateChanged);
        }
        
    }
    
}
void ASFBaseAIController::PreInitializeComponents()
{
    Super::PreInitializeComponents();
    UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

//Combat
USFEnemyCombatComponent* ASFBaseAIController::GetCombatComponent() const
{
    if (IsValid(CombatComponent))
    {
        return CombatComponent;
    }
    return nullptr;
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
            BTComp->PauseLogic(Reason); 
            StopMovement(); 
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

#pragma region Team
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


#pragma endregion 

#pragma region Rotation
void ASFBaseAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
    if (bSuppressControlRotationUpdates)
        return;

    if (CurrentRotationMode != EAIRotationMode::ControllerYaw)
        return;

    APawn* MyPawn = GetPawn();
    if (!MyPawn) return;

    // ✅ 이동 상태 변화 감지 (매 프레임)
    const float Speed = MyPawn->GetVelocity().Size2D();
    const bool bIsMoving = Speed > 10.f;

    if (bIsMoving != bWasMovingLastFrame)
    {
        SetMovementBasedRotation(bIsMoving);
        bWasMovingLastFrame = bIsMoving;
    }

    const FVector FocalPoint = GetFocalPoint();
    if (!FAISystem::IsValidLocation(FocalPoint))
        return;

    FVector ToTarget = FocalPoint - MyPawn->GetActorLocation();
    ToTarget.Z = 0.f;

    if (ToTarget.IsNearlyZero())
        return;

    const FRotator TargetRot = ToTarget.Rotation();
    const FRotator NewControlRot = FMath::RInterpTo(GetControlRotation(), TargetRot, DeltaTime, 10.f);

    SetControlRotation(NewControlRot);
}

void ASFBaseAIController::SetRotationMode(EAIRotationMode NewMode)
{
    if (CurrentRotationMode == NewMode)
        return;

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
        bWasMovingLastFrame = false;
        break;

    case EAIRotationMode::ControllerYaw:
        MoveComp->bOrientRotationToMovement = false;

        // ✅ 현재 이동 상태에 맞게 초기 설정
        const float Speed = GetPawn()->GetVelocity().Size2D();
        const bool bIsMoving = Speed > 10.f;
        bWasMovingLastFrame = bIsMoving;

        SetMovementBasedRotation(bIsMoving);

        if (TargetActor && !GetFocusActor())
        {
            SetFocus(TargetActor, EAIFocusPriority::Gameplay);
        }
        break;
    }
}

void ASFBaseAIController::SetMovementBasedRotation(bool bIsMoving)
{
    ACharacter* Char = Cast<ACharacter>(GetPawn());
    if (!Char) return;

    UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement();
    if (!MoveComp) return;

    if (bIsMoving)
    {
        // 이동 중: Strafe (타겟을 바라보며 이동)
        MoveComp->bUseControllerDesiredRotation = true;
        MoveComp->RotationRate = FRotator(0.f, MovingRotationRate, 0.f);  // ✅ Blueprint 변수 사용
    }
    else
    {
        // 정지 중: TurnInPlaceComponent가 각도에 따라 자동으로 처리
        // - 작은 각도: EnableNaturalRotation(true) → 자연스러운 회전
        // - 큰 각도: EnableNaturalRotation(false) → TurnInPlace 애니메이션
        // 초기 상태는 회전 중지 (TurnInPlaceComponent가 판단할 때까지)
        MoveComp->bUseControllerDesiredRotation = false;
        MoveComp->RotationRate = FRotator::ZeroRotator;
    }
}

void ASFBaseAIController::DisableTurnInPlaceFor(float Duration)
{
    if (!GetWorld())
        return;

    DisableTurnInPlaceUntilTime = GetWorld()->GetTimeSeconds() + Duration;
}

bool ASFBaseAIController::IsTurningInPlace() const
{
    if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn()))
    {
        return ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_TurningInPlace);
    }
    return false;
}


bool ASFBaseAIController::CanTurnInPlace() const
{
    if (!GetWorld())
        return true;

    return GetWorld()->GetTimeSeconds() >= DisableTurnInPlaceUntilTime;
}

void ASFBaseAIController::EnsureRotationModeReset()
{
    
    SetRotationMode(EAIRotationMode::ControllerYaw);
    DisableTurnInPlaceFor(0.1f);

}

void ASFBaseAIController::RestoreRotationAfterAbility()
{
    bSuppressControlRotationUpdates = false;

    // ❗ TurnInPlace 중이면 복구 금지 - Ability 종료 후 TurnInPlace 침범 방지
    if (IsTurningInPlace())
    {
        return;
    }

    if (TargetActor)
    {
        SetRotationMode(EAIRotationMode::ControllerYaw);
        SetFocus(TargetActor, EAIFocusPriority::Gameplay);
    }
    else
    {
        SetRotationMode(EAIRotationMode::MovementDirection);
    }

    DisableTurnInPlaceFor(1.0f);
}

void ASFBaseAIController::OnAbilityStateChanged(const FGameplayTag Tag, int32 NewCount)
{

    if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn()))
    {
        if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_TurningInPlace))
        {
            return;
        }
    }

    if (NewCount > 0)
    {
        // Ability 시작: 컨트롤러 회전 업데이트 억제
        bSuppressControlRotationUpdates = true;
        SetRotationMode(EAIRotationMode::None);
    }
    else
    {
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(RotationRestoreTimerHandle);
            GetWorld()->GetTimerManager().SetTimer(RotationRestoreTimerHandle, this, &ThisClass::RestoreRotationAfterAbility, 0.15f, false);
        }
    }
}

#pragma endregion

