// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Enemy/SFEnemyData.h"
#include "Components/ActorComponent.h"
#include "Templates/Function.h"
#include "SFStateReactionComponent.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStateChanged, FGameplayTag, StateTag);

// State시 행동 struct
USTRUCT()
struct FStateReaction
{
	GENERATED_BODY()
	
	TFunction<void()> OnStart;
	TFunction<void()> OnEnd;
};




UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFStateReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	static USFStateReactionComponent* FindStateReactionComponent(const AActor* Actor) {return (Actor ? Actor->FindComponentByClass<USFStateReactionComponent>() : nullptr);};
	
	void Initialize(UAbilitySystemComponent* InASC);


public:
	// 상태 시작/종료 Delegate
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged OnStateStart;

	UPROPERTY(BlueprintAssignable)
	FOnStateChanged OnStateEnd;
	

protected:
	UPROPERTY()
	UAbilitySystemComponent* ASC;
	
	UPROPERTY()
	TMap<FGameplayTag, FStateReaction> StateReactionMap;

	UPROPERTY()
	UAnimInstance* AnimInstance;

	

private:
	void OnTagChanged(const FGameplayTag Tag, int32 NewCount);

	void BindingTagsDelegate();

	void MappingStateReaction();
	
	// 각 Tag별 함수
	// 상태별 함수
	//Parried
	void StartParried();
	void EndParried();
	//Stunned
	void StartStunned();
	void EndStunned();
	//Knockback
	void StartKnockback();
	void EndKnockback();
	//KnockDown
	void StartKnockdown();
	void EndKnockdown();
	//Groggy
	void StartGroggy();
	void EndGroggy();
	//Hit
	void StartHitReact();
	void EndHitReact();
	//Death
	void StartDead();
	void EndDead();

	
	
	
};
