// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "SFAnimNotify_SendGameplayEvent.generated.h"

/**
 * 
 */
UCLASS(meta=(DisplayName="Send Gameplay Event"))
class SF_API USFAnimNotify_SendGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	USFAnimNotify_SendGameplayEvent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Gameplay Event")
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, Category = "Gameplay Event")
	FGameplayEventData EventData;
};
