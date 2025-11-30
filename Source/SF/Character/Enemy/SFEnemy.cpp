// Fill out your copyright notice in the Description page of Project Settings.


#include "SFEnemy.h"

#include "AbilitySystem/SFAbilitySet.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/SFCombatSet.h"
#include "AbilitySystem/Attributes/SFPrimarySet.h"
#include "AbilitySystem/Attributes/Enemy/SFCombatSet_Enemy.h"
#include "AbilitySystem/Attributes/Enemy/SFPrimarySet_Enemy.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/SFAIGameplayTags.h"
#include "Character/SFPawnData.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Component/SFEnemyMovementComponent.h"
#include "Component/SFStateReactionComponent.h"
#include "System/SFGameInstance.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(SFEnemy)

ASFEnemy::ASFEnemy(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

	// Ability
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<USFAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	//AttributeSet
	PrimarySet = CreateDefaultSubobject<USFPrimarySet_Enemy>(TEXT("PrimarySet"));
	CombatSet = CreateDefaultSubobject<USFCombatSet_Enemy>(TEXT("CombatSet"));
	
	SetNetUpdateFrequency(100.f);
	//사실 PlayerState에서 Ability세팅할때랑 똑같이 세팅을 라이라에서는 하는것 같다

	StateReactionComponent = ObjectInitializer.CreateDefaultSubobject<USFStateReactionComponent>(this, TEXT("StateReactionComponent"));
	StateReactionComponent->SetIsReplicated(true);
	
	
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
		return;
	}

	if (!EnemyPawnData)
	{
		return;
	}

	USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(this);
	if (!PawnExtComp)
	{
		return;
	}

	PawnExtComp->SetPawnData(EnemyPawnData);
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

#pragma region InitializeComponents
void ASFEnemy::InitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	USFPawnExtensionComponent* PawnExtComp = FindComponentByClass<USFPawnExtensionComponent>();
	if (!PawnExtComp)
	{
		return;
	}
	
	PawnExtComp->InitializeAbilitySystem(AbilitySystemComponent, this);
	InitializeAttributeSet(PawnExtComp);
	GrantAbilitiesFromPawnData();
	
}

void ASFEnemy::InitializeAttributeSet(USFPawnExtensionComponent* PawnExtComp)
{
	const USFEnemyData* EnemyData = PawnExtComp->GetPawnData<USFEnemyData>();
	if (!EnemyData)
	{
		return;
	}
	USFGameInstance* GI = Cast<USFGameInstance>(GetWorld()->GetGameInstance());
	if (!GI)
	{
		return;
	}
	
	const FEnemyAttributeData* AttrData = GI->EnemyDataMap.Find(EnemyData->EnemyID);
	TMap<FGameplayTag, float> AttrMap;
	if (AttrData)
	{
		AttrMap.Add(SFGameplayTags::Data_MaxHealth, AttrData->MaxHealth);
		AttrMap.Add(SFGameplayTags::Data_AttackPower, AttrData->AttackPower);
		AttrMap.Add(SFGameplayTags::Data_MoveSpeed, AttrData->MoveSpeed);
		AttrMap.Add(SFGameplayTags::Data_Defense, AttrData->Defense);
		AttrMap.Add(SFGameplayTags::Data_CriticalDamage, AttrData->CriticalDamage);
		AttrMap.Add(SFGameplayTags::Data_CriticalChance, AttrData->CriticalChance);
		AttrMap.Add(SFGameplayTags::Data_Enemy_MaxStagger, AttrData->MaxStagger);
	}
	if (IsValid(InitializeEffect))
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
			InitializeEffect, 1.0f, EffectContext);

		if (AttrMap.Num() > 0)
		{
			for (auto Pair : AttrMap)
			{
				SpecHandle.Data->SetSetByCallerMagnitude(Pair.Key, Pair.Value);
			}
		}
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
	}
}

void ASFEnemy::InitializeStateReactionComponent()
{
	if (IsValid(StateReactionComponent) && IsValid(AbilitySystemComponent))
	{
		StateReactionComponent->Initialize(AbilitySystemComponent);
	}
}

void ASFEnemy::InitializeMovementComponent()
{
	if (USFEnemyMovementComponent* MoveComp =  Cast<USFEnemyMovementComponent>(GetMovementComponent()))
	{
		MoveComp->InitializeMovementComponent();

		if (PrimarySet->GetMoveSpeedAttribute().IsValid())
		{
			MoveComp ->MaxWalkSpeed = PrimarySet->GetMoveSpeed();
		}
	}
	
}


#pragma endregion

void ASFEnemy::GrantAbilitiesFromPawnData()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	if (!AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		return;
	}

	USFPawnExtensionComponent* PawnExtComp = FindComponentByClass<USFPawnExtensionComponent>();
	if (!PawnExtComp)
	{
		return;
	}

	const USFPawnData* PawnData = PawnExtComp->GetPawnData<USFPawnData>();
	if (!PawnData)
	{
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
		
	}

}
