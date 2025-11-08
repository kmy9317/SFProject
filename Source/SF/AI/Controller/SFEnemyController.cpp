// Fill out your copyright notice in the Description page of Project Settings.


#include "SFEnemyController.h"

#include "AI/StateMachine/SFStateMachine.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Components/GameFrameworkComponentManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFEnemyController)


ASFEnemyController::ASFEnemyController(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick  = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
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
	BindingStateMachine(InPawn);

	
}

void ASFEnemyController::OnUnPossess()
{
	UnBindingStateMachine();
	Super::OnUnPossess();
}
#pragma region BehaviorTree


void ASFEnemyController::SetBehaviorTree(UBehaviorTree* NewBehaviorTree)
{
	if (!NewBehaviorTree)
	{
		return;
	}

	if (!CachedBehaviorTreeComponent)
	{
		return;
	}

	UBehaviorTree* CurrentTree = CachedBehaviorTreeComponent->GetCurrentTree();
	if (CurrentTree == NewBehaviorTree)
	{
		return;
	}
	

	RunBehaviorTree(NewBehaviorTree);
}

void ASFEnemyController::BindingStateMachine(const APawn* InPawn)
{
	if (!InPawn)
	{
		return;
	}

	CachedBehaviorTreeComponent = Cast<UBehaviorTreeComponent>(GetBrainComponent());
	if (!CachedBehaviorTreeComponent)
	{
		return;
	}

	CachedBlackboardComponent = GetBlackboardComponent();
	if (!CachedBlackboardComponent)
	{
		
		return;
	}

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
	{
		return;
	}

	USFStateMachine* StateMachine = USFStateMachine::FindStateMachineComponent(ControlledPawn);
	if (StateMachine)
	{
		StateMachine->OnChangeTreeDelegate.RemoveAll(this);
		StateMachine->OnStopTreeDelegate.RemoveAll(this);
		
	}
}

void ASFEnemyController::StopBehaviorTree()
{
	if (!CachedBehaviorTreeComponent)
	{
		return;
	}
	
	CachedBehaviorTreeComponent->StopTree();
}

void ASFEnemyController::ChangeBehaviorTree(FGameplayTag GameplayTag)
{
	if (!GameplayTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("ASFEnemyController::ChangeBehaviorTree - Invalid GameplayTag"));
		return;
	}

	UBehaviorTree* NewTree = BehaviorTreeContainer.GetBehaviourTree(GameplayTag);
	if (NewTree)
	{
		UE_LOG(LogTemp, Log, TEXT("ASFEnemyController::ChangeBehaviorTree - Changing to tree for tag: %s"),
			*GameplayTag.ToString());
		SetBehaviorTree(NewTree);
	}

}


#pragma endregion






