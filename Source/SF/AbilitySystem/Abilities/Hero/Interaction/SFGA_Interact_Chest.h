#pragma once

#include "CoreMinimal.h"
#include "SFGA_Interact_Object.h"
#include "SFGA_Interact_Chest.generated.h"

/**
 * (일반 강화, 어빌리티 진화) 선택지 표시를 위한 어빌리티 
 */
UCLASS()
class SF_API USFGA_Interact_Chest : public USFGA_Interact_Object
{
	GENERATED_BODY()

public:
	USFGA_Interact_Chest(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="SF|Interaction")
	TSoftClassPtr<UUserWidget> ActivateWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> ActivateWidget;
};
