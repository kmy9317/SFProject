// Fill out your copyright notice in the Description page of Project Settings.


#include "SFAnimNotifyState_PerformTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "KismetTraceUtils.h"
#include "SFLogChannels.h"
#include "Character/SFCharacterBase.h"
#include "Components/BoxComponent.h"
#include "Equipment/SFEquipmentTags.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapons/Actor/SFEquipmentBase.h"

USFAnimNotifyState_PerformTrace::USFAnimNotifyState_PerformTrace(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	// 에디터 프리뷰에서는 트레이스 실행 방지
	bShouldFireInEditor = false;
#endif
}

void USFAnimNotifyState_PerformTrace::NotifyBegin(USkeletalMeshComponent* MeshComponent, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComponent, Animation, TotalDuration, EventReference);

	// 지정된 네트워크 역할에서만 실행
	// 기본값 ROLE_Authority = 서버에서만 트레이스 수행
	if (MeshComponent->GetOwnerRole() != ExecuteNetRole)
	{
		return;
	}

	if (ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(MeshComponent->GetOwner()))
	{
		if (USFEquipmentComponent* EquipmentComponent = SFCharacter->FindComponentByClass<USFEquipmentComponent>())
		{
			ASFEquipmentBase* EquipmentActor = Cast<ASFEquipmentBase>(EquipmentComponent->GetFirstEquippedActorBySlot(WeaponSlotTag));
			if (EquipmentActor && EquipmentComponent->IsSlotEquipmentMatchesTag(WeaponSlotTag, SFGameplayTags::EquipmentTag_Weapon))
			{
				CachedWeaponActor = EquipmentActor;
				
				// 트레이스 방식에 따라 Transform 소스 선택
				if (TraceParams.TraceMethod == ESFTraceMethod::ComponentSweep)
				{
					PreviousTraceTransform = CachedWeaponActor->MeshComponent->GetComponentTransform();
					PreviousDebugTransform = CachedWeaponActor->TraceDebugCollision->GetComponentTransform();
				}
				else
				{
					PreviousTraceTransform = CachedWeaponActor->TraceDebugCollision->GetComponentTransform();
					PreviousDebugTransform = PreviousTraceTransform;
				}
				PreviousSocketTransform = CachedWeaponActor->MeshComponent->GetSocketTransform(TraceParams.TraceSocketName);
			}
		}
	}

	if (CachedWeaponActor.IsValid())
	{
#if UE_EDITOR
		if (TraceDebugParams.bLogTraceInfo)
		{
			FVector WeaponExtent = CachedWeaponActor->TraceDebugCollision->GetScaledBoxExtent();
			FVector ScaleDiff = TraceShapeParams.ExtentScale - FVector::OneVector;
			FVector CalculatedOffset = WeaponExtent * ScaleDiff;

			UE_LOG(LogSF, Warning, TEXT("========== Trace Box Info =========="));
			UE_LOG(LogSF, Warning, TEXT("Weapon BoxExtent: X=%.1f, Y=%.1f, Z=%.1f"), 
				WeaponExtent.X, WeaponExtent.Y, WeaponExtent.Z);
			UE_LOG(LogSF, Warning, TEXT("ExtentScale: X=%.1f, Y=%.1f, Z=%.1f"), 
				TraceShapeParams.ExtentScale.X, TraceShapeParams.ExtentScale.Y, TraceShapeParams.ExtentScale.Z);
			UE_LOG(LogSF, Warning, TEXT("------------------------------------"));
			UE_LOG(LogSF, Warning, TEXT("한쪽 방향 확장용 PivotOffset:"));
			UE_LOG(LogSF, Warning, TEXT("  + 방향: (%.1f, %.1f, %.1f)"), 
				CalculatedOffset.X, CalculatedOffset.Y, CalculatedOffset.Z);
			UE_LOG(LogSF, Warning, TEXT("  - 방향: (%.1f, %.1f, %.1f)"), 
				-CalculatedOffset.X, -CalculatedOffset.Y, -CalculatedOffset.Z);
			UE_LOG(LogSF, Warning, TEXT("===================================="));
		}
#endif
	}

	CachedHitActors.Empty();
}

void USFAnimNotifyState_PerformTrace::NotifyTick(USkeletalMeshComponent* MeshComponent, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComponent, Animation, FrameDeltaTime, EventReference);

	if (MeshComponent->GetOwnerRole() != ExecuteNetRole)
	{
		return;
	}

	if (CachedWeaponActor.IsValid() == false)
	{
		return;
	}

	PerformTrace(MeshComponent);
}

void USFAnimNotifyState_PerformTrace::NotifyEnd(USkeletalMeshComponent* MeshComponent, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComponent, Animation, EventReference);

	if (MeshComponent->GetOwnerRole() != ExecuteNetRole)
	{
		return;
	}

	// 무기 액터 유효성 검증
	if (CachedWeaponActor.IsValid() == false)
	{
		return;
	}

	PerformTrace(MeshComponent);
}

void USFAnimNotifyState_PerformTrace::PerformTrace(USkeletalMeshComponent* MeshComponent)
{
	FTransform CurrentSocketTransform = CachedWeaponActor->MeshComponent->GetSocketTransform(TraceParams.TraceSocketName);

	// 이전 프레임과 현재 프레임 간 소켓 이동 거리 계산
	float Distance = (PreviousSocketTransform.GetLocation() - CurrentSocketTransform.GetLocation()).Length();

	// TargetDistance 기준으로 서브스텝 개수 결정
	// 예: Distance=50, TargetDistance=20 → SubStepCount=3
	int SubStepCount = FMath::CeilToInt(Distance / TraceParams.TargetDistance);
	if (SubStepCount <= 0)
	{
		return;
	}
	
	// 각 서브스텝의 비율 (0~1 사이 값)
	float SubstepRatio = 1.f / SubStepCount;

	// 트레이스 방식에 따라 Transform 소스 선택
	FTransform CurrentTraceTransform;
	FTransform CurrentDebugTransform;

	if (TraceParams.TraceMethod == ESFTraceMethod::ComponentSweep)
	{
		CurrentTraceTransform = CachedWeaponActor->MeshComponent->GetComponentTransform();
		CurrentDebugTransform = CachedWeaponActor->TraceDebugCollision->GetComponentTransform();
	}
	else
	{
		CurrentTraceTransform = CachedWeaponActor->TraceDebugCollision->GetComponentTransform();
		CurrentDebugTransform = CurrentTraceTransform;
	}

	// 트레이스 박스 크기 계산
	FVector BoxExtent = GetTraceBoxExtent();
	FCollisionShape CollisionShape = FCollisionShape::MakeBox(BoxExtent);

	// 오브젝트 타입 설정
	FCollisionObjectQueryParams ObjectQueryParams;
	for (EObjectTypeQuery ObjectType : TraceParams.ObjectTypes)
	{
		ObjectQueryParams.AddObjectTypesToQuery(UEngineTypes::ConvertToCollisionChannel(ObjectType));
	}

	TArray<FHitResult> FinalHitResults;

	for (int32 i = 0; i < SubStepCount; i++)
	{
		// 서브스텝의 시작/끝 Transform 계산 (Dual Quaternion 보간으로 부드러운 회전)
		FTransform StartTraceTransform = UKismetMathLibrary::TLerp(PreviousTraceTransform, CurrentTraceTransform, SubstepRatio * i, ELerpInterpolationMode::DualQuatInterp);
		FTransform EndTraceTransform = UKismetMathLibrary::TLerp(PreviousTraceTransform, CurrentTraceTransform, SubstepRatio * (i + 1), ELerpInterpolationMode::DualQuatInterp);

		// 평균 Transform (회전에 사용)
		FTransform AverageTraceTransform = UKismetMathLibrary::TLerp(StartTraceTransform, EndTraceTransform, 0.5f, ELerpInterpolationMode::DualQuatInterp);

		// Pivot 오프셋 적용 (로컬 → 월드 변환)
		FVector StartLocation = StartTraceTransform.GetLocation();
		FVector EndLocation = EndTraceTransform.GetLocation();

		if (!TraceShapeParams.PivotOffset.IsZero())
		{
			StartLocation += StartTraceTransform.GetRotation().RotateVector(TraceShapeParams.PivotOffset);
			EndLocation += EndTraceTransform.GetRotation().RotateVector(TraceShapeParams.PivotOffset);
		}
		
		TArray<FHitResult> HitResults;

		if (TraceParams.TraceMethod == ESFTraceMethod::ComponentSweep)
		{
			// ComponentSweepMulti 방식
			FComponentQueryParams Params = FComponentQueryParams::DefaultComponentQueryParams;
			Params.bReturnPhysicalMaterial = true;
			TArray<AActor*> IgnoredActors = { CachedWeaponActor.Get(), CachedWeaponActor->GetOwner() };
			Params.AddIgnoredActors(IgnoredActors);

			MeshComponent->GetWorld()->ComponentSweepMulti(
				HitResults,
				CachedWeaponActor->MeshComponent,
				StartLocation,
				EndLocation,
				AverageTraceTransform.GetRotation(),
				Params
			);
		}
		else
		{
			// BoxSweep 방식
			FCollisionQueryParams Params;
			Params.bReturnPhysicalMaterial = true;
			Params.AddIgnoredActor(CachedWeaponActor.Get());
			Params.AddIgnoredActor(CachedWeaponActor->GetOwner());

			MeshComponent->GetWorld()->SweepMultiByObjectType(
				HitResults,
				StartLocation,
				EndLocation,
				AverageTraceTransform.GetRotation(),
				ObjectQueryParams,
				CollisionShape,
				Params
			);
		}

		for (const FHitResult& HitResult : HitResults)
		{
			AActor* HitActor = HitResult.GetActor();
			if (CachedHitActors.Contains(HitActor) == false)
			{
				CachedHitActors.Add(HitActor);
				FinalHitResults.Add(HitResult);
			}
		}

#if UE_EDITOR
		if (GIsEditor && TraceDebugParams.bDrawDebugShape)
		{
			FColor Color = (HitResults.Num() > 0) ? TraceDebugParams.HitColor : TraceDebugParams.TraceColor;

			FTransform StartDebugTransform = UKismetMathLibrary::TLerp(PreviousDebugTransform, CurrentDebugTransform, SubstepRatio * i, ELerpInterpolationMode::DualQuatInterp);
			FTransform EndDebugTransform = UKismetMathLibrary::TLerp(PreviousDebugTransform, CurrentDebugTransform, SubstepRatio * (i + 1), ELerpInterpolationMode::DualQuatInterp);
			FTransform AverageDebugTransform = UKismetMathLibrary::TLerp(StartDebugTransform, EndDebugTransform, 0.5f, ELerpInterpolationMode::DualQuatInterp);

			FVector StartDebugLocation = StartDebugTransform.GetLocation();
			FVector EndDebugLocation = EndDebugTransform.GetLocation();

			if (!TraceShapeParams.PivotOffset.IsZero())
			{
				StartDebugLocation += StartDebugTransform.GetRotation().RotateVector(TraceShapeParams.PivotOffset);
				EndDebugLocation += EndDebugTransform.GetRotation().RotateVector(TraceShapeParams.PivotOffset);
			}
			
			DrawDebugSweptBox(
			   MeshComponent->GetWorld(),
			   StartDebugLocation,
			   EndDebugLocation,
			   AverageDebugTransform.GetRotation().Rotator(),
			   (TraceParams.TraceMethod == ESFTraceMethod::ComponentSweep)
				   ? CachedWeaponActor->TraceDebugCollision->GetScaledBoxExtent()
				   : BoxExtent,
			   Color,
			   false,
			   2.f
		   );
		}
#endif
	}

	PreviousTraceTransform = CurrentTraceTransform;
	PreviousDebugTransform = CurrentDebugTransform;
	PreviousSocketTransform = CurrentSocketTransform;
	
	if (FinalHitResults.Num() > 0)
	{
		// 히트 결과를 TargetDataHandle로 패키징
		FGameplayAbilityTargetDataHandle TargetDataHandle;

		for (const FHitResult& HitResult : FinalHitResults)
		{
			// 각 히트 결과를 SingleTargetHit 데이터로 변환
			FGameplayAbilityTargetData_SingleTargetHit* NewTargetData = new FGameplayAbilityTargetData_SingleTargetHit();
			NewTargetData->HitResult = HitResult;
			TargetDataHandle.Add(NewTargetData);
		}

		FGameplayEventData EventData;
		EventData.TargetData = TargetDataHandle; 
		EventData.Instigator = CachedWeaponActor.Get();    

		if (EventTag.IsValid())
		{
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MeshComponent->GetOwner(), EventTag, EventData);
		}
	}
}

FVector USFAnimNotifyState_PerformTrace::GetTraceBoxExtent() const
{
	if (!CachedWeaponActor.IsValid())
	{
		return TraceShapeParams.BoxExtent;
	}

	if (TraceShapeParams.bUseWeaponDefaultExtent)
	{
		return CachedWeaponActor->TraceDebugCollision->GetScaledBoxExtent() * TraceShapeParams.ExtentScale;
	}

	return TraceShapeParams.BoxExtent;
}


