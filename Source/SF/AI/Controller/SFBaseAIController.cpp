#include "SFBaseAIController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AI/StateMachine/SFStateMachine.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Net/UnrealNetwork.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Character/Enemy/Component/SFStateReactionComponent.h"
#include "Team/SFTeamTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFBaseAIController)

ASFBaseAIController::ASFBaseAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bIsInCombat = false;
    TargetActor = nullptr;
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
    {
        return;
    }
    // 스폰 위치 저장
    SpawnLocation = InPawn->GetActorLocation();
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

