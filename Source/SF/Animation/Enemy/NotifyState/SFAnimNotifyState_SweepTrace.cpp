// Fill out your copyright notice in the Description page of Project Settings.


#include "SFAnimNotifyState_SweepTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/Hero/AnimNotify/SFAnimNotify_SendGameplayEvent.h"
#include "DrawDebugHelpers.h"

void USFAnimNotifyState_SweepTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	// 타이머 초기화
	TimeSinceLastTrace = 0.f;
	HitActors.Reset();
}

void USFAnimNotifyState_SweepTrace::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float DeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, DeltaTime, EventReference);

	AActor* Owner = MeshComp->GetOwner();

	if (!Owner || !Owner->HasAuthority())
		return;

	UWorld* World = Owner->GetWorld();
	if (!World)
		return;

	// TraceInterval 체크
	TimeSinceLastTrace += DeltaTime;
	if (TimeSinceLastTrace < TraceInterval)
		return;

	// Trace 수행 후 타이머 리셋
	TimeSinceLastTrace = 0.f;

	for (const FSFSweepSocketChain& Chain : SocketChains)
	{
		const int32 NumNodes = Chain.Nodes.Num();
		if (NumNodes < 2)
			continue;

		for (int32 i = 0; i < NumNodes - 1; ++i)
		{
			const FSFSweepSocketNode& A = Chain.Nodes[i];
			const FSFSweepSocketNode& B = Chain.Nodes[i + 1];

			// 시작 위치 
			const FVector Start = MeshComp->GetSocketLocation(A.SocketName);
			//끝
			const FVector End   = MeshComp->GetSocketLocation(B.SocketName);

			//A 소켓에서 B 소켓으로 이동하는 구간의 평균 크기를 하자
			const float Radius = (A.Radius + B.Radius) * 0.5f;

			TArray<FHitResult> Hits;

			FCollisionQueryParams Params;
			Params.AddIgnoredActor(Owner);
			Params.bTraceComplex = false;

			bool bHit = false;

			switch (Chain.TraceType)
			{
			case ESFSweepTraceType::Line:
				bHit = World->LineTraceMultiByChannel(
					Hits, Start, End, TraceChannel, Params);
				break;

			case ESFSweepTraceType::Sphere:
				bHit = World->SweepMultiByChannel(
					Hits, Start, End, FQuat::Identity,
					TraceChannel,
					FCollisionShape::MakeSphere(Radius),
					Params);
				break;

			case ESFSweepTraceType::Capsule:
			{
				const float HalfHeight =
					FVector::Dist(Start, End) * 0.5f + Chain.HalfHeightPadding;

				bHit = World->SweepMultiByChannel(
					Hits, Start, End, FQuat::Identity,
					TraceChannel,
					FCollisionShape::MakeCapsule(Radius, HalfHeight),
					Params);
				break;
			}
			}

			// 디버그 시각화
			if (bIsDebug)
			{
				FColor DebugColor = bHit ? FColor::Green : FColor::Red;
				float DebugDuration = 2.0f;
				float DebugThickness = 2.0f;

				switch (Chain.TraceType)
				{
				case ESFSweepTraceType::Line:
					// Line Trace: 선으로 표시
					DrawDebugLine(World, Start, End, DebugColor, false, DebugDuration, 0, DebugThickness);
					break;

				case ESFSweepTraceType::Sphere:
					// Sphere Sweep: 시작/끝 지점에 구체 표시
					DrawDebugSphere(World, Start, Radius, 12, DebugColor, false, DebugDuration, 0, DebugThickness);
					DrawDebugSphere(World, End, Radius, 12, DebugColor, false, DebugDuration, 0, DebugThickness);
					DrawDebugLine(World, Start, End, DebugColor, false, DebugDuration, 0, DebugThickness);
					break;

				case ESFSweepTraceType::Capsule:
				{
					const float HalfHeight = FVector::Dist(Start, End) * 0.5f + Chain.HalfHeightPadding;
					const FVector Center = (Start + End) * 0.5f;
					const FVector Direction = (End - Start).GetSafeNormal();
					const FQuat Rotation = FRotationMatrix::MakeFromZ(Direction).ToQuat();

					// Capsule Sweep: 캡슐 형태로 표시
					DrawDebugCapsule(World, Center, HalfHeight, Radius, Rotation, DebugColor, false, DebugDuration, 0, DebugThickness);
					break;
				}
				}
				
			}

			if (!bHit)
				continue;

			for (const FHitResult& Hit : Hits)
			{
				AActor* HitActor = Hit.GetActor();
				if (!HitActor || HitActor == Owner)
					continue;

				if (!HitActor->IsA(APawn::StaticClass()))
				{
					continue;
				}
				if (HitActors.Contains(HitActor))
					continue;

				HitActors.Add(HitActor);

				// Gameplay Event 전달
				FGameplayEventData EventData;
				EventData.EventTag = EventTag;
				EventData.Instigator = Owner;
				EventData.Target = HitActor;
				UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
				if (!ASC)
					continue;
				FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
				ContextHandle.AddHitResult(Hit);
				EventData.ContextHandle = ContextHandle;

				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
					Owner,
					EventTag,
					EventData
				);
			}
		}
	}
}


void USFAnimNotifyState_SweepTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	HitActors.Reset();
}
