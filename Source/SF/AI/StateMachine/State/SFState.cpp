// Fill out your copyright notice in the Description page of Project Settings.


#include "SFState.h"
#include "AI/StateMachine/SFStateMachine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFState)

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
	if (StateInstance)
	{
		return; // 이미 인스턴스가 있음
	}

	if (!StateClass)
	{
		UE_LOG(LogTemp, Error, TEXT("FSFStateSpec::EnsureInstance - StateClass is null"));
		return;
	}

	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("FSFStateSpec::EnsureInstance - Owner is null"));
		return;
	}

	// 재귀 생성 방지를 위한 정적 집합
	static TSet<TSubclassOf<USFState>> CreatingStates;

	if (CreatingStates.Contains(StateClass))
	{
		UE_LOG(LogTemp, Error, TEXT("FSFStateSpec::EnsureInstance - Recursive state creation detected for class: %s"),
			*StateClass->GetName());
		return;
	}

	// 생성 중 표시
	CreatingStates.Add(StateClass);

	// State 인스턴스 생성
	StateInstance = NewObject<USFState>(Owner, StateClass);

	// 생성 완료 표시
	CreatingStates.Remove(StateClass);

	if (StateInstance)
	{
		if (USFStateMachine* SM = Cast<USFStateMachine>(Owner))
		{
			StateInstance->Initialize(SM, OwnerActor);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("FSFStateSpec::EnsureInstance - Owner is not a StateMachine"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FSFStateSpec::EnsureInstance - Failed to create state instance for class: %s"),
			*StateClass->GetName());
	}
}

void FSFStateSpec::Enter()
{
	if (!StateInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("FSFStateSpec::Enter - StateInstance is null"));
		return;
	}

	// Status 업데이트
	Status = EStateStatus::Begin;
	StateInstance->OnEnter();
}

void FSFStateSpec::Exit()
{
	if (!StateInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSFStateSpec::Exit - StateInstance is null"));
		return;
	}

	if (Status == EStateStatus::Running || Status == EStateStatus::Begin)
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

