#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "SFInputConfig.h"
#include "SFInputComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	USFInputComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	template <class UserClass, typename FuncType>
	void BindNativeAction(const USFInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound);

	template <class UserClass, typename StartedFuncType, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const USFInputConfig* InputConfig, UserClass* Object, StartedFuncType StartedFunc, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles);
	
};

template <class UserClass, typename FuncType>
void USFInputComponent::BindNativeAction(const USFInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound)
{
	check(InputConfig);

	// InputConfig는 활성화 가능한 InputAction을 담고 있음
	// 만약 InputConfig에 없는 InputAction을 Binding시키면 nullptr을 반환하여 바인딩하는데 실패
	if (const UInputAction* IA = InputConfig->FindNativeInputActionForTag(InputTag, bLogIfNotFound))
	{
		BindAction(IA, TriggerEvent, Object, Func);
	}
}

template <class UserClass, typename StartedFuncType, typename PressedFuncType, typename ReleasedFuncType>
void USFInputComponent::BindAbilityActions(const USFInputConfig* InputConfig, UserClass* Object, StartedFuncType StartedFunc, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles)
{
	check(InputConfig);

	// AbilityAction에 대해서는 그냥 모든 InputAction에 다 바인딩
	for (const FSFInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if (StartedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Started, Object, StartedFunc, Action.InputTag).GetHandle());
			}

			if (PressedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, PressedFunc, Action.InputTag).GetHandle());
			}

			if (ReleasedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag).GetHandle());
			}
		}
	}
}