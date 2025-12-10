// Fill out your copyright notice in the Description page of Project Settings.


#include "SFEnemy.h"

#include "AbilitySystem/SFAbilitySet.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/SFCombatSet.h"
#include "AbilitySystem/Attributes/SFPrimarySet.h"
#include "AbilitySystem/Attributes/Enemy/SFCombatSet_Enemy.h"
#include "AbilitySystem/Attributes/Enemy/SFPrimarySet_Enemy.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/Controller/SFEnemyController.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/SFPawnData.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Component/SFEnemyMovementComponent.h"
#include "Component/SFEnemyWidgetComponent.h"
#include "Component/SFStateReactionComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameModes/SFEnemyManagerComponent.h"
#include "GameModes/SFGameState.h"
#include "System/SFGameInstance.h"
#include "Net/UnrealNetwork.h"


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

	StateReactionComponent = ObjectInitializer.CreateDefaultSubobject<USFStateReactionComponent>(this, TEXT("StateReactionComponent"));
	StateReactionComponent->SetIsReplicated(true);

	EnemyWidgetComponent = ObjectInitializer.CreateDefaultSubobject<USFEnemyWidgetComponent>(this, TEXT("EnemyWidgetComponent"));
	EnemyWidgetComponent->SetupAttachment(RootComponent);
	EnemyWidgetComponent->SetIsReplicated(true);

	SetNetUpdateFrequency(100.f);
	//사실 PlayerState에서 Ability세팅할때랑 똑같이 세팅을 라이라에서는 하는것 같다
	
	
}

void ASFEnemy::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 EnemyManager에 등록
	if (HasAuthority())
	{
		if (ASFGameState* SFGameState = GetWorld()->GetGameState<ASFGameState>())
		{
			if (USFEnemyManagerComponent* EnemyManager = SFGameState->GetEnemyManager())
			{
				EnemyManager->RegisterEnemy(this);
			}
		}
	}
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
		AttrMap.Add(SFGameplayTags::Data_Enemy_GuardRange, AttrData->GuardRange);
		AttrMap.Add(SFGameplayTags::Data_Enemy_SightRadius, AttrData->SightRadius);
		AttrMap.Add(SFGameplayTags::Data_Enemy_LoseSightRadius, AttrData->LoseSightRadius);
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

FGenericTeamId ASFEnemy::GetGenericTeamId() const
{
	// [수정] 변수명 충돌 방지를 위해 Controller -> EnemyController 로 변경
	if (ASFEnemyController* EnemyController = Cast<ASFEnemyController>(GetController()))
	{
		return EnemyController->GetGenericTeamId();
	}
	else
	{
		return Super::GetGenericTeamId();    
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

void ASFEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	
	DOREPLIFETIME(ThisClass, LastAttacker);
}

void ASFEnemy::SetLastAttacker(AActor* Attacker)
{

	if (!HasAuthority())
	{
		return;
	}
	
	LastAttacker = Attacker;

	// [추가] 공격자가 있다면 컨트롤러에게 타겟 강제 변경 요청
	if (Attacker)
	{
		// 내 컨트롤러를 ASFEnemyController로 캐스팅하여 함수 호출
		if (ASFEnemyController* AIC = Cast<ASFEnemyController>(GetController()))
		{
			AIC->SetTargetForce(Attacker);
		}
	}

	OnRep_LastAttacker();
}

void ASFEnemy::OnRep_LastAttacker()
{
	
	if (LastAttacker && LastAttacker->HasLocalNetOwner())
	{
		if (EnemyWidgetComponent)
		{
			EnemyWidgetComponent->MarkAsAttackedByLocalPlayer();
		}
	}
}

void ASFEnemy::TurnCollisionOn()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
}

void ASFEnemy::TurnCollisionOff()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
}
