// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Templates/Function.h"
#include "SFEnemyMovementComponent.generated.h"



/**
 * 
 */
UCLASS()
class SF_API USFEnemyMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	static USFEnemyMovementComponent* FindSFEnemyMovementComponent(const AActor* Actor) {return (Actor ? Actor->FindComponentByClass<USFEnemyMovementComponent>() : nullptr);}

	void InitializeMovementComponent();

	
protected:
	void InternalDisableMovement();
	void InternalEnableMovement();
	void MappingStateFunction();
	UFUNCTION()
	void OnStateStart(FGameplayTag StateTag);
	UFUNCTION()
	void OnStateEnd(FGameplayTag StateTag);


	// State 함수
	void StartStateParried();
	void EndStateParried();

	void StartStateStunned();
	void EndStateStunned();

	void StartStateKnockback();
	void EndStateKnockback();

	void StartStateKnockdown();
	void EndStateKnockdown();

	void StartStateGroggy();
	void EndStateGroggy();

	void StartStateHitReact();
	void EndStateHitReact();

	void StartStateDead();
	void EndStateDead();
	
protected:
	EMovementMode PreviousMovementMode = MOVE_Walking;
	

	TMap<FGameplayTag, TFunction<void()>> StateStartMap;

	
	TMap<FGameplayTag, TFunction<void()>> StateEndMap;

};

