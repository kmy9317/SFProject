// Fill out your copyright notice in the Description page of Project Settings.


#include "SFState.h"

#include "AI/StateMachine/SFStateMachine.h"

void FSFStateHandle::GenerateNewHandle()
{
	static int32 GHandle = 1;
	Handle = GHandle++;
}
#pragma region Spec
bool FSFStateSpec::IsActive() const
{
	return Status == EStateStatus::Running;
}

bool FSFStateSpec::IsValid() const
{
	return StateClass != nullptr && Handle.IsValid();
}

void FSFStateSpec::EnsureInstance(UObject* Owner, AActor* OwnerActor)
{
	if (!StateInstance && StateClass)
	{
		StateInstance = NewObject<USFState>(Owner, StateClass);
		
		if (StateInstance)
		{
			if (USFStateMachine* SM = Cast<USFStateMachine>(Owner))
			{
				StateInstance->Initialize(SM, OwnerActor);
			}
		}
	}

}

void FSFStateSpec::Enter()
{
	if (!StateInstance)
	{
		return;
	}
	// Status 업데이트
	Status = EStateStatus::Begin;
	StateInstance->OnEnter();
}

void FSFStateSpec::Exit()
{
	if (StateInstance && (Status == EStateStatus::Running || Status == EStateStatus::Begin))
	{
		StateInstance->OnExit();
		Status = EStateStatus::End;
	}
}
void FSFStateSpec::Pause()
{
	if (StateInstance && Status == EStateStatus::Running)
	{
		StateInstance->OnPause();
		Status = EStateStatus::Paused;
	}
}

void FSFStateSpec::Resume()
{
	if (StateInstance && Status == EStateStatus::Paused)
	{
		StateInstance->OnResume();
		Status = EStateStatus::Running;
	}
}

void FSFStateSpec::Update(float DeltaTime)
{
	if (StateInstance && Status == EStateStatus::Running)
	{
		StateInstance->OnUpdate(DeltaTime);
	}
	else if  (StateInstance && Status == EStateStatus::Begin)
	{
		Status = EStateStatus::Running;
		StateInstance->OnUpdate(DeltaTime);
	}
	
}

bool FSFStateSpec::CanTransitionTo(TSubclassOf<USFState> ToStateClass) const
{
	if (StateInstance)
	{
		return StateInstance->CanTransitionTo(ToStateClass);
	}
	return false;  
}

#pragma endregion 



void USFState::Initialize(USFStateMachine* InStateMachine, AActor* InOwner)
{
	OwnerActor =  InOwner;
	StateMachine = InStateMachine;
}

void USFState::OnEnter_Implementation()
{
}

void USFState::OnExit_Implementation()
{
}

void USFState::OnUpdate_Implementation(float DeltaTime)
{
}

void USFState::OnPause_Implementation()
{
}
void USFState::OnResume_Implementation()
{
}
bool USFState::CanTransitionTo_Implementation(TSubclassOf<USFState> ToStateClass) const
{
	return true;
}

