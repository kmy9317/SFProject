// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/SFCharacterBase.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Interface/SFLockOnInterface.h"
#include "SFEnemy.generated.h"

class USFCombatSet_Enemy;
class USFPrimarySet_Enemy;
class USFCombatSet;
class USFPawnData;
class USFPrimarySet;

UCLASS(Blueprintable)
class SF_API ASFEnemy : public ASFCharacterBase, public ISFLockOnInterface
{
	GENERATED_BODY()

public:
	ASFEnemy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void InitializeComponents();

	virtual void PostInitializeComponents() override;
	
	// ASC 초기화
	virtual void InitializeAbilitySystem();
	
	//Attrtibute 초기화
	virtual void InitializeAttributeSet(USFPawnExtensionComponent* PawnExtComp);

	virtual void InitializeMovementComponent();

	virtual FGenericTeamId GetGenericTeamId() const override;

	void TurnCollisionOn();
	void TurnCollisionOff();
	
	// ISFLockOnInterface Implementation
	virtual bool CanBeLockedOn() const override;
	virtual TArray<FName> GetLockOnSockets() const override;
	virtual void OnSelectedAsTarget(bool bSelected) override;

protected:

	virtual void OnAbilitySystemInitialized() override;
	//PawnData에 있는 AbilitySet GIVE
	void GrantAbilitiesFromPawnData();

	// Collision 처리를 위한 Tag 감지
	void RegisterCollisionTagEvents();
	void OnCollisionTagChanged(const FGameplayTag Tag, int32 NewCount);

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

	UPROPERTY(VisibleAnywhere, Category="Component")
	TObjectPtr<class USFEnemyWidgetComponent> EnemyWidgetComponent;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|InitializeEffect")
	TSubclassOf<UGameplayEffect> InitializeEffect;

	// 기본 락온 소켓 이름 (spine_02 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|LockOn")
	FName DefaultLockOnSocketName = FName("spine_02");
};
