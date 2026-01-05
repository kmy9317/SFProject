// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Templates/Function.h"
#include "SFEnemyMovementComponent.generated.h"


UCLASS()
class SF_API USFEnemyMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	static USFEnemyMovementComponent* FindSFEnemyMovementComponent(const AActor* Actor) {return (Actor ? Actor->FindComponentByClass<USFEnemyMovementComponent>() : nullptr);}


	void OnMoveSpeedChanged(const FOnAttributeChangeData& OnAttributeChangeData);
	virtual void InitializeMovementComponent();

protected:
	void MappingStateFunction();

	// StateReactionComponent 제거 - 직접 Tag 감지
	void RegisterStateTagEvents(class UAbilitySystemComponent* ASC);
	void OnStateTagChanged(const FGameplayTag Tag, int32 NewCount);

	virtual void InternalDisableMovement();

	virtual void InternalEnableMovement();

	void OnStateStart(FGameplayTag StateTag);
	void OnStateEnd(FGameplayTag StateTag);
	
	//  멈춰야 하는 상태 (Hard CC)
	void StartStateParried();
	void EndStateParried();

	void StartStateStunned();
	void EndStateStunned();

	void StartStateGroggy();
	void EndStateGroggy();

	void StartStateDead();
	void EndStateDead();

	//  움직여야 하는 상태 (Soft CC)
	void StartStateKnockback();
	void EndStateKnockback();

	void StartStateKnockdown();
	void EndStateKnockdown();

	void StartStateHitReact();
	void EndStateHitReact();
    
protected:
	EMovementMode PreviousMovementMode = MOVE_Walking;
    
	TMap<FGameplayTag, TFunction<void()>> StateStartMap;
	TMap<FGameplayTag, TFunction<void()>> StateEndMap;
};

