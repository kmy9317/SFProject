// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "SFEnemyComponent.generated.h"


class USFStateMachine;
class UBehaviorTree;
class AAIController;
/**
 * 
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFEnemyComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	USFEnemyComponent(const FObjectInitializer& ObjectInitializer =  FObjectInitializer::Get());
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = EnemyComponent)
	static USFEnemyComponent* FindSFEnemyComponent(const AActor* Actor) {return (Actor ? Actor->FindComponentByClass<USFEnemyComponent>() : nullptr);}
	
#pragma region IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const { return NAME_ActorFeatureName;}
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
public:
	static const FName NAME_ActorFeatureName; 
	
#pragma endregion

#pragma region InitializeComponents

public:
	virtual void InitializeAI();

	virtual void InitalizeStateMachineComponent();

#pragma endregion
	
};
