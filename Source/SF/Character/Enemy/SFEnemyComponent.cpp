// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/SFEnemyComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFEnemyComponent)


const FName USFEnemyComponent::NAME_ActorFeatureName("Enemy");


USFEnemyComponent::USFEnemyComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
	
}

void USFEnemyComponent::OnRegister()
{
	Super::OnRegister();
	
	if (!GetPawn<APawn>())
	{
		return;
	}

	// InitState에 등록
	RegisterInitStateFeature();
}

void USFEnemyComponent::BeginPlay()
{
	Super::BeginPlay();

	

}

void USFEnemyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 등록 해제 
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}


#pragma region GameFrameWorkInitState
bool USFEnemyComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState) const
{
	check(Manager);

	
	return IGameFrameworkInitStateInterface::CanChangeInitState(Manager, CurrentState, DesiredState);
}

void USFEnemyComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState)
{
	IGameFrameworkInitStateInterface::HandleChangeInitState(Manager, CurrentState, DesiredState);
}

void USFEnemyComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	IGameFrameworkInitStateInterface::OnActorInitStateChanged(Params);
}

void USFEnemyComponent::CheckDefaultInitialization()
{
	IGameFrameworkInitStateInterface::CheckDefaultInitialization();
}
#pragma endregion

#pragma region AI

void USFEnemyComponent::InitializeAI()
{
}

#pragma endregion 