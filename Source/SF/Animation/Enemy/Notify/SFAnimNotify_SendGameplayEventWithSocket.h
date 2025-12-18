// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "SFAnimNotify_SendGameplayEventWithSocket.generated.h"

/**
 * AnimNotify that sends a Gameplay Event with Socket location data
 * Used for Enemy attacks that need precise location information (e.g., Dragon Stomp)
 */
UCLASS(meta=(DisplayName="Send Gameplay Event With Socket"))
class SF_API USFAnimNotify_SendGameplayEventWithSocket : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	USFAnimNotify_SendGameplayEventWithSocket(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	/** Gameplay Event Tag to send */
	UPROPERTY(EditAnywhere, Category = "Gameplay Event")
	FGameplayTag EventTag;

	/** Socket name to get location from (e.g., "foot_l", "foot_r"). If empty, uses Actor location */
	UPROPERTY(EditAnywhere, Category = "Gameplay Event")
	FName SocketName;
};

