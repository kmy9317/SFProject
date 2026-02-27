#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "SFAbilityTask_WaitCancelInput.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCancelInputDelegate);

UCLASS()
class SF_API USFAbilityTask_WaitCancelInput : public UAbilityTask
{
	GENERATED_BODY()
public:
	USFAbilityTask_WaitCancelInput(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks",
		meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static USFAbilityTask_WaitCancelInput* WaitCancelInput(UGameplayAbility* OwningAbility);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable)
	FCancelInputDelegate OnCancelInput;

private:
	UFUNCTION()
	void OnCancelCallback();

	FDelegateHandle DelegateHandle;
};
