// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/SFEnemyComponent.h"

#include "AIController.h"
#include "AI/Controller/SFEnemyController.h"
#include "AI/StateMachine/SFStateMachine.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "System/SFInitGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFEnemyComponent)


const FName USFEnemyComponent::NAME_ActorFeatureName("Enemy");


USFEnemyComponent::USFEnemyComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
	
}

void USFEnemyComponent::OnRegister()
{
	Super::OnRegister();
	
	if (!GetPawn<APawn>())
	{
		return;
	}

	// InitState에 등록
	RegisterInitStateFeature();
}

void USFEnemyComponent::BeginPlay()
{
	Super::BeginPlay();
	
	//다른 컴포넌트들의 초기화 상태 감지 
	BindOnActorInitStateChanged(NAME_None, FGameplayTag(), false);
	
	ensure(TryToChangeInitState(SFGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void USFEnemyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 등록 해제 
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}


#pragma region GameFrameWorkInitState
bool USFEnemyComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState) const
{
	check(Manager);
	APawn* Pawn  =  GetPawn<APawn>();

	
	if (!CurrentState.IsValid() && DesiredState == SFGameplayTags::InitState_Spawned)
	{
		if (Pawn)
		{
			return true;
		}
	}
	//Spawn -> DataAvailable -> AIController 있는지 체크랑 PawnData 있는지 체크
	if ( CurrentState == SFGameplayTags::InitState_Spawned && DesiredState == SFGameplayTags::InitState_DataAvailable)
	{
		if (!GetController<AAIController>())
		{
			return false;
		}

		if (USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn)) //PawnExtensionData가 필요 
		{
			if (!PawnExtComp->GetPawnData<USFPawnData>())
			{
				return false;
			}
		}
		else
		{
			return false;
		}
		
		return true;
	}
	else if (CurrentState == SFGameplayTags::InitState_DataAvailable && DesiredState == SFGameplayTags::InitState_DataInitialized)
	{
		//다른 것들도 다 Initialized 되어있는지 체크
		return Manager->HasFeatureReachedInitState(Pawn, USFPawnExtensionComponent::NAME_ActorFeatureName, SFGameplayTags::InitState_DataInitialized);
	}
	else if (CurrentState == SFGameplayTags::InitState_DataInitialized && DesiredState == SFGameplayTags::InitState_GameplayReady)
	{
		return true;
	}
	return false;
	
}
// 각 상태별 ACTION
void USFEnemyComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState)
{
	if (CurrentState == SFGameplayTags::InitState_DataAvailable && DesiredState == SFGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn  = GetPawn<APawn>();
		if (!ensure(Pawn))
		{
			return;
		}

		USFPawnExtensionComponent* PawnExtComp =  USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
		if (!ensure(PawnExtComp))
		{
			return;
		}

		const USFPawnData* PawnData = PawnExtComp->GetPawnData<USFPawnData>();
		if (!ensure(PawnData))
		{
			return;
		}
		
	}
	else if (CurrentState == SFGameplayTags::InitState_DataInitialized && DesiredState == SFGameplayTags::InitState_GameplayReady)
	{
		InitializeAI();
		InitalizeStateMachineComponent();
	}	
}

void USFEnemyComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	// PawnExtension 상태가 DataInitialized여야 시도 가능 
	if (Params.FeatureName == USFPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == SFGameplayTags::InitState_DataInitialized)
		{
			CheckDefaultInitialization();
		}
	}
}

void USFEnemyComponent::CheckDefaultInitialization()
{
	//다른 컴포넌트들 체크 
	CheckDefaultInitializationForImplementers();

	static const TArray<FGameplayTag> StateChain = {
		SFGameplayTags::InitState_Spawned,
		SFGameplayTags::InitState_DataAvailable,
		SFGameplayTags::InitState_DataInitialized,
		SFGameplayTags::InitState_GameplayReady
	};

	
	ContinueInitStateChain(StateChain);
}
#pragma endregion

#pragma region AI

void USFEnemyComponent::InitializeAI()
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		UE_LOG(LogTemp, Error, TEXT("USFEnemyComponent::InitializeAI - Pawn is null"));
		return;
	}

	AAIController* AIController = GetController<AAIController>();
	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("USFEnemyComponent::InitializeAI - AIController not found for %s"), *Pawn->GetName());
		return;
	}

	USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (!PawnExtComp)
	{
		UE_LOG(LogTemp, Error, TEXT("USFEnemyComponent::InitializeAI - PawnExtensionComponent not found for %s"), *Pawn->GetName());
		return;
	}

	const USFEnemyData* EnemyData = PawnExtComp->GetPawnData<USFEnemyData>();
	if (!EnemyData)
	{
		UE_LOG(LogTemp, Error, TEXT("USFEnemyComponent::InitializeAI - EnemyData not found for %s"), *Pawn->GetName());
		return;
	}

	ASFEnemyController* EnemyController = Cast<ASFEnemyController>(AIController);
	if (!EnemyController)
	{
		UE_LOG(LogTemp, Warning, TEXT("USFEnemyComponent::InitializeAI - AIController is not SFEnemyController for %s"), *Pawn->GetName());
		return;
	}

	EnemyController->SetBehaviourContainer(EnemyData->BehaviourContainer);
	UE_LOG(LogTemp, Log, TEXT("USFEnemyComponent::InitializeAI - Successfully initialized AI for %s"), *Pawn->GetName());
}

void USFEnemyComponent::InitalizeStateMachineComponent()
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		UE_LOG(LogTemp, Error, TEXT("USFEnemyComponent::InitalizeStateMachineComponent - Pawn is null"));
		return;
	}

	USFStateMachine* CachedStateMachine = USFStateMachine::FindStateMachineComponent(Pawn);
	if (!IsValid(CachedStateMachine))
	{
		UE_LOG(LogTemp, Warning, TEXT("USFEnemyComponent::InitalizeStateMachineComponent - StateMachine not found for %s"), *Pawn->GetName());
		return;
	}

	USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (!PawnExtComp)
	{
		UE_LOG(LogTemp, Error, TEXT("USFEnemyComponent::InitalizeStateMachineComponent - PawnExtensionComponent not found for %s"), *Pawn->GetName());
		return;
	}

	const USFEnemyData* EnemyData = PawnExtComp->GetPawnData<USFEnemyData>();
	if (!EnemyData)
	{
		UE_LOG(LogTemp, Error, TEXT("USFEnemyComponent::InitalizeStateMachineComponent - EnemyData not found for %s"), *Pawn->GetName());
		return;
	}

	// State 등록
	CachedStateMachine->RegisterStates(EnemyData->StateContainer);

	// 기본 State 활성화
	if (EnemyData->DefaultStateTag.IsValid())
	{
		bool bActivated = CachedStateMachine->ActivateStateByTag(EnemyData->DefaultStateTag);
		if (bActivated)
		{
			UE_LOG(LogTemp, Log, TEXT("USFEnemyComponent::InitalizeStateMachineComponent - Activated default state %s for %s"),
				*EnemyData->DefaultStateTag.ToString(), *Pawn->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("USFEnemyComponent::InitalizeStateMachineComponent - Failed to activate default state %s for %s"),
				*EnemyData->DefaultStateTag.ToString(), *Pawn->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("USFEnemyComponent::InitalizeStateMachineComponent - No default state tag set for %s"), *Pawn->GetName());
	}
}

#pragma endregion
