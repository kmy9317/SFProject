// Fill out your copyright notice in the Description page of Project Settings.


#include "SFEnemy.h"

#include "AbilitySystem/SFAbilitySet.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/SFCombatSet.h"
#include "AbilitySystem/Attributes/SFPrimarySet.h"
#include "Character/SFPawnData.h"
#include "Character/SFPawnExtensionComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(SFEnemy)

ASFEnemy::ASFEnemy(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

	// Ability
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<USFAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	//AttributeSet
	PrimarySet = CreateDefaultSubobject<USFPrimarySet>(TEXT("PrimarySet"));
	CombatSet = CreateDefaultSubobject<USFCombatSet>(TEXT("CombatSet"));
	
	SetNetUpdateFrequency(100.f);
	//사실 PlayerState에서 Ability세팅할때랑 똑같이 세팅을 라이라에서는 하는것 같다
	
}

void ASFEnemy::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
}

void ASFEnemy::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!HasAuthority())
	{
		return;
	}

	if (!NewController)
	{
		UE_LOG(LogTemp, Error, TEXT("ASFEnemy::PossessedBy - NewController is null for %s"), *GetName());
		return;
	}

	if (!EnemyPawnData)
	{
		UE_LOG(LogTemp, Error, TEXT("ASFEnemy::PossessedBy - EnemyPawnData is null for %s"), *GetName());
		return;
	}

	USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(this);
	if (!PawnExtComp)
	{
		UE_LOG(LogTemp, Error, TEXT("ASFEnemy::PossessedBy - PawnExtensionComponent not found for %s"), *GetName());
		return;
	}

	PawnExtComp->SetPawnData(EnemyPawnData);
	UE_LOG(LogTemp, Log, TEXT("ASFEnemy::PossessedBy - Successfully set PawnData for %s"), *GetName());
}


UAbilitySystemComponent* ASFEnemy::GetAbilitySystemComponent() const
{
	if (USFPawnExtensionComponent* PawnExtComp = FindComponentByClass<USFPawnExtensionComponent>())
	{
		if (USFAbilitySystemComponent* ASC = PawnExtComp->GetSFAbilitySystemComponent())
		{
			return ASC;
		}
	}
	return AbilitySystemComponent;
}

void ASFEnemy::InitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("ASFEnemy::InitializeAbilitySystem - AbilitySystemComponent is null for %s"), *GetName());
		return;
	}

	USFPawnExtensionComponent* PawnExtComp = FindComponentByClass<USFPawnExtensionComponent>();
	if (!PawnExtComp)
	{
		UE_LOG(LogTemp, Error, TEXT("ASFEnemy::InitializeAbilitySystem - PawnExtensionComponent not found for %s"), *GetName());
		return;
	}

	PawnExtComp->InitializeAbilitySystem(AbilitySystemComponent, this);
	GrantAbilitiesFromPawnData();

	UE_LOG(LogTemp, Log, TEXT("ASFEnemy::InitializeAbilitySystem - Successfully initialized for %s"), *GetName());
}

void ASFEnemy::GrantAbilitiesFromPawnData()
{
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("ASFEnemy::GrantAbilitiesFromPawnData - AbilitySystemComponent is null for %s"), *GetName());
		return;
	}

	if (!AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		UE_LOG(LogTemp, Verbose, TEXT("ASFEnemy::GrantAbilitiesFromPawnData - Not authoritative for %s"), *GetName());
		return;
	}

	USFPawnExtensionComponent* PawnExtComp = FindComponentByClass<USFPawnExtensionComponent>();
	if (!PawnExtComp)
	{
		UE_LOG(LogTemp, Error, TEXT("ASFEnemy::GrantAbilitiesFromPawnData - PawnExtensionComponent not found for %s"), *GetName());
		return;
	}

	const USFPawnData* PawnData = PawnExtComp->GetPawnData<USFPawnData>();
	if (!PawnData)
	{
		UE_LOG(LogTemp, Error, TEXT("ASFEnemy::GrantAbilitiesFromPawnData - PawnData is null for %s"), *GetName());
		return;
	}

	// PawnData에 있는 모든 Ability들을 GiveToAbilitySystem
	int32 GrantedCount = 0;
	for (const USFAbilitySet* AbilitySet : PawnData->AbilitySets)
	{
		if (AbilitySet)
		{
			AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr, this);
			GrantedCount++;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ASFEnemy::GrantAbilitiesFromPawnData - Null AbilitySet in PawnData for %s"), *GetName());
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ASFEnemy::GrantAbilitiesFromPawnData - Granted %d ability sets for %s"), GrantedCount, *GetName());
}
