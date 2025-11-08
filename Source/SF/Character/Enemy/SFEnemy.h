// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/SFCharacterBase.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "SFEnemy.generated.h"

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


protected:
	
	virtual void PostInitializeComponents() override;

	// ASC 초기화
	void InitializeAbilitySystem();

	//PawnData에 있는 AbilitySet GIVE
	void GrantAbilitiesFromPawnData();

protected:
	UPROPERTY(VisibleAnywhere, Category= "Abilites")
	TObjectPtr<USFAbilitySystemComponent> AbilitySystemComponent;

	// 캐릭터의 기본 AttributeSet
	UPROPERTY()
	TObjectPtr<USFPrimarySet> PrimarySet;
	
	// 전투 관련 AttributeSet
	UPROPERTY()
	TObjectPtr<USFCombatSet> CombatSet;

	//PawnData를 직접 배치해야함 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PawnData")
	TObjectPtr<const USFPawnData> EnemyPawnData;

	// 향후 제거를 위해 
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
};
