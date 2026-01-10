// Fill out your copyright notice in the Description page of Project Settings.
#include "SFEnemy.h"

#include "SFEnemyGameplayTags.h"
#include "AbilitySystem/SFAbilitySet.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/SFCombatSet.h"
#include "AbilitySystem/Attributes/SFPrimarySet.h"
#include "AbilitySystem/Attributes/Enemy/SFCombatSet_Enemy.h"
#include "AbilitySystem/Attributes/Enemy/SFPrimarySet_Enemy.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/Controller/SFEnemyController.h"
#include "Animation/Enemy/SFEnemyAnimInstance.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/SFPawnData.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Component/SFEnemyMovementComponent.h"
#include "Component/SFEnemyWidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameModes/SFEnemyManagerComponent.h"
#include "GameModes/SFGameState.h"
#include "GameModes/SFStageManagerComponent.h"
#include "System/SFGameInstance.h"
#include "Net/UnrealNetwork.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(SFEnemy)

ASFEnemy::ASFEnemy(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

	// Ability
	
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<USFAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	//AttributeSet
	PrimarySet = CreateDefaultSubobject<USFPrimarySet_Enemy>(TEXT("PrimarySet"));
	CombatSet = CreateDefaultSubobject<USFCombatSet_Enemy>(TEXT("CombatSet"));

	EnemyWidgetComponent = ObjectInitializer.CreateDefaultSubobject<USFEnemyWidgetComponent>(this, TEXT("EnemyWidgetComponent"));
	EnemyWidgetComponent->SetupAttachment(RootComponent);
	EnemyWidgetComponent->SetIsReplicated(false);

	SetNetUpdateFrequency(100.f);
	//사실 PlayerState에서 Ability세팅할때랑 똑같이 세팅을 라이라에서는 하는것 같다

    bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->bUseControllerDesiredRotation = false;
        MoveComp->bOrientRotationToMovement = false;
    	
        MoveComp->RotationRate = FRotator::ZeroRotator;

    	MoveComp->MaxAcceleration = 500.0f;
    	MoveComp->BrakingDecelerationWalking = 1024.0f;
    	MoveComp->GroundFriction = 8.0f;
    }
	
}

UAbilitySystemComponent* ASFEnemy::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASFEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		if (ASFGameState* SFGameState = GetWorld()->GetGameState<ASFGameState>())
		{
			if (USFEnemyManagerComponent* EnemyManager = SFGameState->GetEnemyManager())
			{
				EnemyManager->RegisterEnemy(this);
			}

			if (const USFEnemyData* Data = Cast<USFEnemyData>(EnemyPawnData))
			{
				if (Data->EnemyType == SFGameplayTags::Enemy_Type_Boss)
				{
					if (USFStageManagerComponent* StageManager = SFGameState->GetStageManager())
					{
						StageManager->RegisterBossActor(this);
					}
				}
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

#pragma region InitializeComponents

void ASFEnemy::InitializeComponents()
{
	InitializeMovementComponent();
	RegisterCollisionTagEvents();
	EnemyWidgetComponent->InitializeWidget();
}

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
	if (!HasAuthority())
	{
		return;
	}
	
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
		
		// [수정] 데이터 테이블 값을 불러오지 않도록 주석 처리
		// 이렇게 하면 컨트롤러(Blueprint)에서 설정한 시야 값을 사용하게 됩니다.
		// AttrMap.Add(SFGameplayTags::Data_Enemy_SightRadius, AttrData->SightRadius);
		// AttrMap.Add(SFGameplayTags::Data_Enemy_LoseSightRadius, AttrData->LoseSightRadius);
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
	if (!HasAuthority())
	{
		return;
	}
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

	if (ASFBaseAIController* EnemyController = Cast<ASFBaseAIController>(GetController()))
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

void ASFEnemy::RegisterCollisionTagEvents()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_PhaseIntro, EGameplayTagEventType::NewOrRemoved)
		   .AddUObject(this, &ThisClass::OnCollisionTagChanged);

		ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Dead, EGameplayTagEventType::NewOrRemoved)
		   .AddUObject(this, &ThisClass::OnCollisionTagChanged);
		
		if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_PhaseIntro) || 
			ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
		{
			TurnCollisionOff();
		}
	}
}

void ASFEnemy::OnCollisionTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (Tag == SFGameplayTags::Character_State_PhaseIntro ||
		Tag == SFGameplayTags::Character_State_Dead)
	{
		if (NewCount > 0)
		{
			TurnCollisionOff();
		}
		else if (Tag == SFGameplayTags::Character_State_PhaseIntro)
		{
			TurnCollisionOn();
		}
	}
}

void ASFEnemy::TurnCollisionOn()
{
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	}
	if (GetMesh())
	{
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetMesh()->SetGenerateOverlapEvents(true);
	}
}

void ASFEnemy::TurnCollisionOff()
{
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	}
	
	if (GetMesh())
	{
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->SetGenerateOverlapEvents(false);
	}
}

FName ASFEnemy::GetName() const
{
	if (EnemyPawnData)
	{
		if (const USFEnemyData* Data = Cast<USFEnemyData>(EnemyPawnData))
		{
			return Data->EnemyName;
		}
	}
	return NAME_None;
}

void ASFEnemy::CheckBossDeath()
{
	if (HasAuthority())
	{
		if (ASFGameState* SFGameState = GetWorld()->GetGameState<ASFGameState>())
		{
			if (USFStageManagerComponent* StageManager = SFGameState->GetStageManager())
			{
				if (const USFEnemyData* Data = Cast<USFEnemyData>(EnemyPawnData))
				{
					if (Data->EnemyType == SFGameplayTags::Enemy_Type_Boss)
					{
							StageManager->RegisterBossActor(nullptr);
					}
				}
			}
		}
	}	
}

void ASFEnemy::OnAbilitySystemInitialized()
{
	Super::OnAbilitySystemInitialized();
	
	InitializeComponents();
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			if (USFEnemyAnimInstance* AnimInst = Cast<USFEnemyAnimInstance>(MeshComp->GetAnimInstance()))
			{
				AnimInst->InitializeWithAbilitySystem(ASC);
			}
		}
		
	}
	
}
