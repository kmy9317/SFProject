// Fill out your copyright notice in the Description page of Project Settings.


#include "SFAbilityTask_WaitForInteractableTraceHit.h"

#include "AbilitySystemComponent.h"
#include "Interaction/SFInteractable.h"

USFAbilityTask_WaitForInteractableTraceHit::USFAbilityTask_WaitForInteractableTraceHit(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

USFAbilityTask_WaitForInteractableTraceHit* USFAbilityTask_WaitForInteractableTraceHit::WaitForInteractableTraceHit(UGameplayAbility* OwningAbility, FSFInteractionQuery InteractionQuery, ECollisionChannel TraceChannel, FGameplayAbilityTargetingLocationInfo StartLocation, float InteractionTraceRange, float InteractionTraceRate, bool bShowDebug)
{
	USFAbilityTask_WaitForInteractableTraceHit* Task = NewAbilityTask<USFAbilityTask_WaitForInteractableTraceHit>(OwningAbility);

	Task->InteractionTraceRange = InteractionTraceRange;    // 상호작용 감지 최대 거리
	Task->InteractionTraceRate = InteractionTraceRate;      // 레이캐스트 수행 주기
	Task->StartLocation = StartLocation;                    // 레이캐스트 시작 위치
	Task->InteractionQuery = InteractionQuery;             // 상호작용 쿼리 정보
	Task->TraceChannel = TraceChannel;                     // 콜리전 채널
	Task->bShowDebug = bShowDebug;                         // 디버그 표시 여부

	return Task;
}

void USFAbilityTask_WaitForInteractableTraceHit::Activate()
{
	Super::Activate();

	SetWaitingOnAvatar();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TraceTimerHandle, this, &ThisClass::PerformTrace, InteractionTraceRate, true);
	}
}

void USFAbilityTask_WaitForInteractableTraceHit::OnDestroy(bool bInOwnerFinished)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TraceTimerHandle);
	}
	Super::OnDestroy(bInOwnerFinished);
}

void USFAbilityTask_WaitForInteractableTraceHit::PerformTrace()
{
	AActor* AvatarActor = Ability->GetCurrentActorInfo()->AvatarActor.Get();
	if (AvatarActor == nullptr)
	{
		return;
	}
	
	// 레이캐스트에서 무시할 액터들 설정(플레이어 자신 + 플레이어에 부착된 엑터들 포함)
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(AvatarActor);
	AvatarActor->GetAttachedActors(ActorsToIgnore, false, true);     

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SFAbilityTask_WaitForInteractableTraceHit), false);
	Params.AddIgnoredActors(ActorsToIgnore);

	// 레이캐스트 시작점 (플레이어 위치)
	FVector TraceStart = StartLocation.GetTargetingTransform().GetLocation();
	FVector TraceEnd;

	// 플레이어 컨트롤러의 카메라 시점을 기반으로 레이캐스트 종료점 계산
	AimWithPlayerController(AvatarActor, Params, TraceStart, InteractionTraceRange, TraceEnd);

	FHitResult HitResult;
	LineTrace(TraceStart, TraceEnd, Params, HitResult);

	TArray<TScriptInterface<ISFInteractable>> Interactables;
	TScriptInterface<ISFInteractable> InteractableActor(HitResult.GetActor());
	if (InteractableActor)
	{
		Interactables.AddUnique(InteractableActor);
	}
	
	TScriptInterface<ISFInteractable> InteractableComponent(HitResult.GetComponent());
	if (InteractableComponent)
	{
		Interactables.AddUnique(InteractableComponent);
	}

	UpdateInteractionInfos(InteractionQuery, Interactables);

	// 디버그
#if ENABLE_DRAW_DEBUG
	if (bShowDebug)
	{
		FColor DebugColor = HitResult.bBlockingHit ? FColor::Red : FColor::Green;
		if (HitResult.bBlockingHit)
		{
			// 히트 지점까지 빨간 라인과 구체 표시
			DrawDebugLine(GetWorld(), TraceStart, HitResult.Location, DebugColor, false, InteractionTraceRate);
			DrawDebugSphere(GetWorld(), HitResult.Location, 5.f, 16, DebugColor, false, InteractionTraceRate);
		}
		else
		{
			// 히트되지 않은 경우 전체 라인을 초록색으로 표시
			DrawDebugLine(GetWorld(), TraceStart, TraceEnd, DebugColor, false, InteractionTraceRate);
		}
	}
#endif // ENABLE_DRAW_DEBUG
}

void USFAbilityTask_WaitForInteractableTraceHit::AimWithPlayerController(const AActor* InSourceActor, const FCollisionQueryParams& Params, const FVector& TraceStart, float MaxRange, FVector& OutTraceEnd, bool bIgnorePitch) const
{
	if (Ability == nullptr)
	{
		return;
	}

	APlayerController* PlayerController = Ability->GetCurrentActorInfo()->PlayerController.Get();
	if (PlayerController == nullptr)
	{
		return;
	}

	FVector CameraStart;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraStart, CameraRotation);

	// 카메라 방향 벡터 계산
	const FVector CameraDirection = CameraRotation.Vector();
	FVector CameraEnd = CameraStart + (CameraDirection * MaxRange);

	// 카메라 방향의 Ray를 플레이어 위치 기준의 상호작용 가능 범위(구체) 이내로 제한
	ClipCameraRayToAbilityRange(CameraStart, CameraDirection, TraceStart, MaxRange, CameraEnd);

	// 카메라에서 클리핑된 종료점까지 레이캐스트 수행
	FHitResult HitResult;
	LineTrace(CameraStart, CameraEnd, Params, HitResult);

	// 히트 결과에 따른 최종 타겟 지점 결정
	// 1. Hit된 물체가 인터렉션 가능 범위(Sphere) 이내라면, Hit 위치를 AdjustedEnd로 정한다.
	// 2. Hit된 물체가 없거나 Hit된 물체가 인터렉션 가능 범위(Sphere)를 벗어 났다면, Hit 위치를 무시하고 Clip된 CameraEnd를 AdjustedEnd로 정한다.
	const bool bUseTraceResult = HitResult.bBlockingHit && (FVector::DistSquared(TraceStart, HitResult.Location) <= (MaxRange * MaxRange));
	const FVector AdjustedEnd = bUseTraceResult ? HitResult.Location : CameraEnd;

	// 플레이어에서 조정된 종료점으로의 방향 벡터 계산
	FVector AdjustedAimDir = (AdjustedEnd - TraceStart).GetSafeNormal();
	if (AdjustedAimDir.IsZero())
	{
		// 방향 벡터가 0이면 카메라 방향을 사용
		AdjustedAimDir = CameraDirection;
	}

	// 플레이어에서 AdjustedAimDir 방향으로 최대 인터렉션 가능 범위(Sphere의 표면)까지 확장한 위치를 TraceEnd로 사용한다.
	OutTraceEnd = TraceStart + (AdjustedAimDir * MaxRange);
}

bool USFAbilityTask_WaitForInteractableTraceHit::ClipCameraRayToAbilityRange(const FVector& CameraLocation, const FVector& CameraDirection, const FVector& AbilityCenter, const float AbilityRange, FVector& OutClippedPosition) const
{
	// 카메라에서 어빌리티 중심점(플레이어)으로의 벡터
	FVector CameraToCenter = AbilityCenter - CameraLocation;

	// 카메라 방향으로의 투영 거리 계산
	float DistanceCameraToDot = FVector::DotProduct(CameraToCenter, CameraDirection);
	
	// 카메라가 어빌리티 중심점을 향하고 있는 경우에만 처리
	if (DistanceCameraToDot >= 0)
	{
		// 카메라에서 어빌리티 중심점까지의 수직 거리 제곱 계산
		float DistanceSquared = CameraToCenter.SizeSquared() - (DistanceCameraToDot * DistanceCameraToDot);
		float RadiusSquared = (AbilityRange * AbilityRange);

		// 카메라 레이가 어빌리티 범위(구체)와 교차하는지 확인
		if (DistanceSquared <= RadiusSquared)
		{
			// 교차점까지의 거리 계산 (피타고라스 정리 응용)
			float DistanceDotToSphere = FMath::Sqrt(RadiusSquared - DistanceSquared);
			float DistanceCameraToSphere = DistanceCameraToDot + DistanceDotToSphere;

			// 클리핑된 위치 계산 (카메라에서 구체 표면까지 -> 두 교차점 중 바깥쪽 교차점에 대해 카메라 방향 * 길이)
			OutClippedPosition = CameraLocation + (DistanceCameraToSphere * CameraDirection);
			return true;
		}
	}
	return false;
}

void USFAbilityTask_WaitForInteractableTraceHit::LineTrace(const FVector& Start, const FVector& End, const FCollisionQueryParams& Params, FHitResult& OutHitResult) const
{
	TArray<FHitResult> HitResults;
	GetWorld()->LineTraceMultiByChannel(HitResults, Start, End, TraceChannel, Params);
	
	if (HitResults.Num() > 0)
	{
		OutHitResult = HitResults[0];
	}
	else
	{
		OutHitResult = FHitResult();
		OutHitResult.TraceStart = Start;
		OutHitResult.TraceEnd = End;
	}
}

void USFAbilityTask_WaitForInteractableTraceHit::UpdateInteractionInfos(const FSFInteractionQuery& InteractQuery, const TArray<TScriptInterface<ISFInteractable>>& Interactables)
{
	TArray<FSFInteractionInfo> NewInteractionInfos;

	// 각 상호작용 가능한 객체에서 상호작용 정보 수집
	for (const TScriptInterface<ISFInteractable>& Interactable : Interactables)
	{
		TArray<FSFInteractionInfo> TempInteractionInfos;
		FSFInteractionInfoBuilder InteractionInfoBuilder(Interactable, TempInteractionInfos);

		// 상호작용 객체에서 정보 수집 (스탯 적용 포함)
		Interactable->GatherPostInteractionInfos(InteractQuery, InteractionInfoBuilder);

		// 수집된 정보들을 검증하고 필터링
		for (FSFInteractionInfo& InteractionInfo : TempInteractionInfos)
		{
			// 부여할 어빌리티가 있는지 확인
			if (InteractionInfo.AbilityToGrant)
			{
				// 해당 어빌리티가 플레이어에게 있는지 확인
				FGameplayAbilitySpec* InteractionAbilitySpec = AbilitySystemComponent->FindAbilitySpecFromClass(InteractionInfo.AbilityToGrant);
				if (InteractionAbilitySpec)
				{
					// 상호작용 가능 여부와 어빌리티 활성화 가능 여부 확인
					if (Interactable->CanInteraction(InteractionQuery) && InteractionAbilitySpec->Ability->CanActivateAbility(InteractionAbilitySpec->Handle, AbilitySystemComponent->AbilityActorInfo.Get()))
					{
						NewInteractionInfos.Add(InteractionInfo);
					}
				}
			}
		}
	}

	// 이전 정보와 비교하여 변화 감지
	bool bInfosChanged = false;
	if (NewInteractionInfos.Num() == CurrentInteractionInfos.Num())
	{
		// 개수가 같으면 내용 비교
		NewInteractionInfos.Sort();

		for (int InfoIndex = 0; InfoIndex < NewInteractionInfos.Num(); InfoIndex++)
		{
			const FSFInteractionInfo& NewInfo = NewInteractionInfos[InfoIndex];
			const FSFInteractionInfo& CurrentInfo = CurrentInteractionInfos[InfoIndex];

			if (NewInfo != CurrentInfo)
			{
				bInfosChanged = true;
				break;
			}
		}
	}
	else
	{
		// 개수가 다르면 변화 있음
		bInfosChanged = true;
	}

	// 변화가 있을 때만 업데이트 수행
	if (bInfosChanged)
	{
		// 이전 객체들의 하이라이트 해제
		HighlightInteractables(CurrentInteractionInfos, false);

		// 새로운 정보로 업데이트
		CurrentInteractionInfos = NewInteractionInfos;

		// 새로운 객체들에 하이라이트 적용
		HighlightInteractables(CurrentInteractionInfos, true);

		// 변화 알림 델리게이트 호출
		InteractableChanged.Broadcast(CurrentInteractionInfos);
	}
}

void USFAbilityTask_WaitForInteractableTraceHit::HighlightInteractables(const TArray<FSFInteractionInfo>& InteractionInfos, bool bShouldHighlight)
{
	TArray<UMeshComponent*> MeshComponents;
	for (const FSFInteractionInfo& InteractionInfo : InteractionInfos)
	{
		if (ISFInteractable* Interactable = InteractionInfo.Interactable.GetInterface())
		{
			Interactable->GetMeshComponents(MeshComponents);
		}
	}

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		MeshComponent->SetRenderCustomDepth(bShouldHighlight);
	}
}
