// Fill out your copyright notice in the Description page of Project Settings.


#include "SFAnimNotify_SendGameplayEventWithSocket.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"

USFAnimNotify_SendGameplayEventWithSocket::USFAnimNotify_SendGameplayEventWithSocket(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	bShouldFireInEditor = false;
#endif
	bIsNativeBranchingPoint = true;
}

void USFAnimNotify_SendGameplayEventWithSocket::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!EventTag.IsValid() || !MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}
	
	UAbilitySystemComponent* ASC = Owner->FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC)
	{
		return;
	}

	
	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	
	ContextHandle.AddInstigator(Owner, Owner);
    
	FVector SocketLocation = FVector::ZeroVector;
	if (SocketName != NAME_None)
	{
		SocketLocation = MeshComp->GetSocketLocation(SocketName);
	}
	else
	{
		SocketLocation = Owner->GetActorLocation();
	}
    
	FHitResult HitResult;
	HitResult.ImpactPoint = SocketLocation;
	HitResult.Location = SocketLocation;
	HitResult.ImpactNormal = FVector::UpVector;
    
	ContextHandle.AddHitResult(HitResult, true);

	
	FGameplayEventData Payload;
	Payload.ContextHandle = ContextHandle;

	
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Owner,
		EventTag,
		Payload
	);
}

