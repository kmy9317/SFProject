// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/SFCharacterBase.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "SFEnemy.generated.h"

class USFCombatSet_Enemy;
class USFPrimarySet_Enemy;
class USFStateReactionComponent;
class USFCombatSet;
class USFPawnData;
class USFPrimarySet;

UCLASS(Blueprintable)
class SF_API ASFEnemy : public ASFCharacterBase
{
	GENERATED_BODY()

public:
	ASFEnemy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PossessedBy(AController* NewController) override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	USFAbilitySystemComponent* GetSFAbilitySystemComponent() const {return AbilitySystemComponent;};
	
	// ASC 초기화
	virtual void InitializeAbilitySystem();
	
	//Attrtibute 초기화
	virtual void InitializeAttributeSet(USFPawnExtensionComponent* PawnExtComp);


	//StateReaction Component 초기화
	virtual void InitializeStateReactionComponent();

	virtual void InitializeMovementComponent();

	virtual FGenericTeamId GetGenericTeamId() const override;

protected:

	//PawnData에 있는 AbilitySet GIVE
	void GrantAbilitiesFromPawnData();

protected:
	UPROPERTY(VisibleAnywhere, Category= "Abilites")
	TObjectPtr<USFAbilitySystemComponent> AbilitySystemComponent;

	// 캐릭터의 기본 AttributeSet
	UPROPERTY()
	TObjectPtr<USFPrimarySet_Enemy> PrimarySet;
	
	// 전투 관련 AttributeSet
	UPROPERTY()
	TObjectPtr<USFCombatSet_Enemy> CombatSet;

	//PawnData를 직접 배치해야함 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PawnData")
	TObjectPtr<const USFPawnData> EnemyPawnData;

	// 향후 제거를 위해 
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;

	UPROPERTY(VisibleAnywhere, Category= "Component")
	TObjectPtr<USFStateReactionComponent> StateReactionComponent;

	UPROPERTY(VisibleAnywhere, Category="Component")
	TObjectPtr<class USFEnemyWidgetComponent> EnemyWidgetComponent;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|InitializeEffect")
	TSubclassOf<UGameplayEffect> InitializeEffect;

	// 마지막 공격자
	UPROPERTY(ReplicatedUsing=OnRep_LastAttacker)
	TObjectPtr<AActor> LastAttacker;

	UFUNCTION()
	void OnRep_LastAttacker();

public:
	void SetLastAttacker(AActor* Attacker);

	
};
