// Fill out your copyright notice in the Description page of Project Settings.


#include "SFAnimNotify_SendGameplayEventWithSocket.h"

#include "AbilitySystemBlueprintLibrary.h"
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

	FGameplayEventData Payload = FGameplayEventData();

	// Socket 위치를 HitResult에 저장하여 전달
	if (SocketName != NAME_None)
	{
		FVector SocketLocation = MeshComp->GetSocketLocation(SocketName);
		
		// HitResult 생성
		FHitResult HitResult;
		HitResult.ImpactPoint = SocketLocation;
		HitResult.Location = SocketLocation;
		HitResult.ImpactNormal = FVector::UpVector; // 기본값: 위쪽
		
		// Context에 HitResult 추가
		Payload.ContextHandle.AddHitResult(HitResult, true);
	}
	else
	{
		// Socket이 없으면 Actor 위치 사용
		if (AActor* Owner = MeshComp->GetOwner())
		{
			FVector ActorLocation = Owner->GetActorLocation();
			
			FHitResult HitResult;
			HitResult.ImpactPoint = ActorLocation;
			HitResult.Location = ActorLocation;
			HitResult.ImpactNormal = FVector::UpVector;
			
			Payload.ContextHandle.AddHitResult(HitResult, true);
		}
	}

	// Gameplay Event 전송
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		MeshComp->GetOwner(),
		EventTag,
		Payload
	);
}

