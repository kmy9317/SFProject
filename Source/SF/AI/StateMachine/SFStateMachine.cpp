// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/StateMachine/SFStateMachine.h"

#include "Character/Enemy/SFEnemyData.h"

#include  UE_INLINE_GENERATED_CPP_BY_NAME(SFStateMachine)

USFStateMachine::USFStateMachine(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USFStateMachine::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (FSFStateSpec* CurrentSpec = FindStateSpec(CurrentStateHandle))
	{
		CurrentSpec->Update(DeltaTime);
	}
}

void USFStateMachine::RegisterStates(FSTStateWrapperContainer StateContainer)
{
	if (StateContainer.States.Num() >0)
	{
		for (auto& State : StateContainer.States)
		{
			RegisterState(State.StateClass, State.StateTag);
		}
	}
}

FSFStateHandle USFStateMachine::RegisterState(TSubclassOf<USFState> StateClass, FGameplayTag StateTag)
{
	if (!StateClass)
	{
		return FSFStateHandle();
	}
	for (FSFStateSpec& Spec : RegisterStateSpecs)
	{
		if (Spec.GetStateClass() == StateClass)  
		{
			return Spec.GetHandle();  
		}
	}
    
	FSFStateSpec NewSpec(StateClass, StateTag);
	RegisterStateSpecs.Add(NewSpec);
	return NewSpec.GetHandle();
}

bool USFStateMachine::ActivateState(FSFStateHandle Handle)
{
	return TransitionToState(Handle);
}

bool USFStateMachine::ActivateStateByClass(TSubclassOf<USFState> StateClass)
{
	if (FSFStateSpec* Spec = FindStateSpecByClass(StateClass))
	{
		return TransitionToState(Spec->GetHandle());
	}
	return false;
}

bool USFStateMachine::ActivateStateByTag(FGameplayTag StateTag)
{
	if (FSFStateSpec* Spec = FindStateSpecByTag(StateTag))
	{
		return TransitionToState(Spec->GetHandle());
	}
	return false;
}

TSubclassOf<USFState> USFStateMachine::GetCurrentStateClass() const
{
	if (const FSFStateSpec* CurrentSpec = FindStateSpec(CurrentStateHandle))
	{
		return CurrentSpec->GetStateClass();
	}
	return nullptr;
}


float USFStateMachine::GetTimeInCurrentState() const
{
    if (const FSFStateSpec* CurrentSpec = FindStateSpec(CurrentStateHandle))
    {
        if (UWorld* World = GetWorld())
        {
            return World->GetTimeSeconds() - CurrentSpec->GetActivationTime();
        }
    }
    return 0.0f;
}

bool USFStateMachine::PushState(FSFStateHandle Handle)
{
	if (!Handle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("USFStateMachine::PushState - Invalid handle"));
		return false;
	}

	FSFStateSpec* NewSpec = FindStateSpec(Handle);
	if (!NewSpec)
	{
		UE_LOG(LogTemp, Warning, TEXT("USFStateMachine::PushState - State spec not found for handle"));
		return false;
	}

	// 1. 현재 State를 Pause하고 Handle만 저장
	if (CurrentStateHandle.IsValid())
	{
		if (FSFStateSpec* CurrentSpec = FindStateSpec(CurrentStateHandle))
		{
			CurrentSpec->Pause();
			ActiveStateHandles.Add(CurrentStateHandle);
		}
	}

	// 2. 새 State를 Running으로 설정
	CurrentStateHandle = Handle;
	EnterState(NewSpec);

	return true;
}

bool USFStateMachine::PopState()
{
	if (ActiveStateHandles.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("USFStateMachine::PopState - No states to pop"));
		return false;
	}

	// 1. 현재 Running State 종료
	if (CurrentStateHandle.IsValid())
	{
		if (FSFStateSpec* CurrentSpec = FindStateSpec(CurrentStateHandle))
		{
			ExitState(CurrentSpec);
		}
	}

	// 2. Stack에서 이전 State Handle 꺼내기
	FSFStateHandle PreviousHandle = ActiveStateHandles.Pop();
	CurrentStateHandle = PreviousHandle;

	// 3. Paused → Running으로 Resume
	if (FSFStateSpec* ResumedSpec = FindStateSpec(CurrentStateHandle))
	{
		ResumedSpec->Resume();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("USFStateMachine::PopState - Failed to find resumed state spec"));
		return false;
	}

	return true;
}

bool USFStateMachine::IsInState(TSubclassOf<USFState> StateClass) const
{
	if (const FSFStateSpec* CurrentSpec = FindStateSpec(CurrentStateHandle))
	{
		return CurrentSpec->GetStateClass() == StateClass;
	}
	return false;
}

void USFStateMachine::BeginPlay()
{
	Super::BeginPlay();
}

FSFStateSpec* USFStateMachine::FindStateSpec(FSFStateHandle Handle)
{
	if (!Handle.IsValid())
	{
		return nullptr;
	}

	// RegisterStateSpecs에서 Handle로 검색
	for (FSFStateSpec& Spec : RegisterStateSpecs)
	{
		if (Spec.GetHandle() == Handle)
		{
			return &Spec;
		}
	}

	return nullptr;
}

const FSFStateSpec* USFStateMachine::FindStateSpec(FSFStateHandle Handle) const
{
	if (!Handle.IsValid())
	{
		return nullptr;
	}

	// RegisterStateSpecs에서 Handle로 검색
	for (const FSFStateSpec& Spec : RegisterStateSpecs)
	{
		if (Spec.GetHandle() == Handle)
		{
			return &Spec;
		}
	}

	return nullptr;
}

FSFStateSpec* USFStateMachine::FindStateSpecByClass(TSubclassOf<USFState> StateClass)
{
	if (!StateClass)
	{
		return nullptr;
	}

	// RegisterStateSpecs에서 Class로 검색
	for (FSFStateSpec& Spec : RegisterStateSpecs)
	{
		if (Spec.GetStateClass() == StateClass)
		{
			return &Spec;
		}
	}

	return nullptr;
}

FSFStateSpec* USFStateMachine::FindStateSpecByTag(FGameplayTag StateTag)
{
	if (!StateTag.IsValid())
	{
		return nullptr;
	}

	// RegisterStateSpecs에서 Tag로 검색
	for (FSFStateSpec& Spec : RegisterStateSpecs)
	{
		if (Spec.GetStateTag() == StateTag)
		{
			return &Spec;
		}
	}
	return nullptr;
}

bool USFStateMachine::TransitionToState(FSFStateHandle NewStateHandle)
{
    // 같은 State로 전환 시도 방지
    if (CurrentStateHandle.IsValid() && CurrentStateHandle == NewStateHandle)
    {
        return false;
    }

    if (!NewStateHandle.IsValid())
    {
        return false;
    }

	FSFStateSpec* NewSpec = FindStateSpec(NewStateHandle);
	if (!NewSpec)
	{
		return false;
	}

    FSFStateSpec* CurSpec = FindStateSpec(CurrentStateHandle);

    // 전환 조건 체크
    if (CurSpec)
    {
        if (!CurSpec->CanTransitionTo(NewSpec->GetStateClass()))
        {
	        return false;
        }
    }

    // 현재 State 종료
    if (CurSpec)
    {
        ExitState(CurSpec);
    }

    CurrentStateHandle = NewStateHandle;

    // 새 State 진입
    EnterState(NewSpec);

    return true;
}
void USFStateMachine::EnterState(FSFStateSpec* Spec)
{
	if (!Spec)
	{
		return;
	}

	Spec->EnsureInstance(this, GetOwner());

	if (UWorld* World = GetWorld())
	{
		Spec->SetActivationTime(World->GetTimeSeconds());
	}

	Spec->Enter();

	// Tick 활성화 (State가 Update를 필요로 할 수 있음)
	SetComponentTickEnabled(true);
}

void USFStateMachine::ExitState(FSFStateSpec* Spec)
{
	if (!Spec)
	{
		return;
	}

	Spec->Exit();

	// 모든 State가 종료되면 Tick 비활성화
	if (!CurrentStateHandle.IsValid() && ActiveStateHandles.Num() == 0)
	{
		SetComponentTickEnabled(false);
	}
}