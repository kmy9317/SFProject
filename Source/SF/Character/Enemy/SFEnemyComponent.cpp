// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/SFEnemyComponent.h"

#include "AIController.h"
#include "SFEnemy.h"
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

	BindOnActorInitStateChanged(NAME_None, FGameplayTag(), false);

	ensure(TryToChangeInitState(SFGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void USFEnemyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}


#pragma region GameFrameWorkInitState
bool USFEnemyComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
    FGameplayTag DesiredState) const
{
    check(Manager);
    APawn* Pawn = GetPawn<APawn>();

    if (!CurrentState.IsValid() && DesiredState == SFGameplayTags::InitState_Spawned)
    {
       if (Pawn)
       {
          return true;
       }
    }
    else if (CurrentState == SFGameplayTags::InitState_Spawned && DesiredState == SFGameplayTags::InitState_DataAvailable)
    {
       // [수정] 서버일 때만 AIController를 체크합니다.
       // 클라이언트는 AIController가 없으므로 이 체크를 건너뛰어야 초기화가 진행됩니다.
       if (Pawn->HasAuthority())
       {
           if (!GetController<AAIController>())
           {
              return false;
           }
       }

       // [공통] PawnData는 서버/클라이언트 모두 필수입니다.
       if (USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
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
       return Manager->HasFeatureReachedInitState(Pawn, USFPawnExtensionComponent::NAME_ActorFeatureName, SFGameplayTags::InitState_DataInitialized);
    }
    else if (CurrentState == SFGameplayTags::InitState_DataInitialized && DesiredState == SFGameplayTags::InitState_GameplayReady)
    {
       return true;
    }

    return false;
}

void USFEnemyComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,FGameplayTag DesiredState)
{
	if (CurrentState == SFGameplayTags ::InitState_DataAvailable && DesiredState == SFGameplayTags::InitState_DataInitialized)
	{
		if (ASFEnemy* Enemy = GetPawn<ASFEnemy>())
		{
			Enemy->InitializeAbilitySystem();
		}
	}
	else if (CurrentState == SFGameplayTags::InitState_DataInitialized && DesiredState == SFGameplayTags::InitState_GameplayReady)
	{
		APawn* Pawn = GetPawn<APawn>();
		if (Pawn && Pawn->HasAuthority())
		{
			InitializeAI();
			InitalizeStateMachineComponent();
		}
	}
}

void USFEnemyComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
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
		return;
	}

	AAIController* AIController = GetController<AAIController>();
	if (!AIController)
	{
		return;
	}

	USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (!PawnExtComp)
	{
		return;
	}

	const USFEnemyData* EnemyData = PawnExtComp->GetPawnData<USFEnemyData>();
	if (!EnemyData)
	{
		return;
	}
	ISFAIControllerInterface* AIControllerInterface = Cast<ISFAIControllerInterface>(AIController);
	if (!AIControllerInterface)
	{
		return;
	}
	AIControllerInterface->InitializeAIController();
			

}

void USFEnemyComponent::InitalizeStateMachineComponent()
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	USFStateMachine* CachedStateMachine = USFStateMachine::FindStateMachineComponent(Pawn);
	if (!IsValid(CachedStateMachine))
	{
		return;
	}
	

	USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (!PawnExtComp)
	{
		return;
	}

	const USFEnemyData* EnemyData = PawnExtComp->GetPawnData<USFEnemyData>();
	if (!EnemyData)
	{
		return;
	}

	// State 등록
	CachedStateMachine->RegisterStates(EnemyData->StateContainer);
	
	if (EnemyData->DefaultStateTag.IsValid())
	{
		CachedStateMachine->ActivateStateByTag(EnemyData->DefaultStateTag);
	}
}

#pragma endregion
