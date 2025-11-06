// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/StateMachine/SFStateMachine.h"

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
	FSFStateSpec* NewSpec = FindStateSpec(Handle);
	if (!NewSpec)
	{
		return false;
	}
	
	if (FSFStateSpec* CurrentSpec = FindStateSpec(CurrentStateHandle))
	{
		CurrentSpec->Pause();
		ActiveStateSpecs.Add(*CurrentSpec);  
	}
    
	// 2. 새 State를 Running으로 설정
	CurrentStateHandle = Handle;
	EnterState(NewSpec);  
    
	return true;
}

bool USFStateMachine::PopState()
{
	if (ActiveStateSpecs.Num() == 0)
	{
		return false;
	}
    
	// 1. 현재 Running State 종료
	if (FSFStateSpec* CurrentSpec = FindStateSpec(CurrentStateHandle))
	{
		ExitState(CurrentSpec);  
	}
    
	// 2. Stack에서 꺼내기
	FSFStateSpec PreviousSpec = ActiveStateSpecs.Pop();
	CurrentStateHandle = PreviousSpec.GetHandle();
    
	// 3. Paused → Running으로 Resume
	if (FSFStateSpec* ResumedSpec = FindStateSpec(CurrentStateHandle))
	{
		ResumedSpec->Resume();
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
    if (CurrentStateHandle.IsValid() && CurrentStateHandle == NewStateHandle)
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
}

void USFStateMachine::ExitState(FSFStateSpec* Spec)
{
	if (Spec)  
	{
		Spec->Exit();
	}
}