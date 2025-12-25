// Fill out your copyright notice in the Description page of Project Settings.


#include "SFAbilityTask_MoveToTargetAndCheckDistance.h"

USFAbilityTask_MoveToTargetAndCheckDistance* USFAbilityTask_MoveToTargetAndCheckDistance::MoveToTargetAndCheckDistance(
	UGameplayAbility* OwningAbility, AActor* Target, float StopDistance, float Duration, UCurveFloat* SpeedCurve)
{
	USFAbilityTask_MoveToTargetAndCheckDistance* Task =NewAbilityTask<USFAbilityTask_MoveToTargetAndCheckDistance>( OwningAbility);

	Task->Target = Target;
	Task->StopDistance = FMath::Square(StopDistance);
	Task->Duration = FMath::Max(Duration, 0.01f);
	Task->SpeedCurve = SpeedCurve;

	return Task;
}

void USFAbilityTask_MoveToTargetAndCheckDistance::Activate()
{
	Super::Activate();
	bTickingTask = true;

	AActor* Avatar = GetAvatarActor();
	if (!Avatar || !Target)
	{
		EndTask();
		return;
	}

	StartTime     = GetWorld()->GetTimeSeconds();
	StartLocation = Avatar->GetActorLocation();

	const FVector TargetLoc = Target->GetActorLocation();
	MoveDirection = (TargetLoc - StartLocation).GetSafeNormal2D();

	TotalDistance = FVector::Dist2D(StartLocation, TargetLoc);
}

void USFAbilityTask_MoveToTargetAndCheckDistance::TickTask(float DeltaTime)
{
	AActor* Avatar = GetAvatarActor();
	if (!Avatar)
	{
		FinishTask(false);
		return;
	}

	const float Elapsed = GetWorld()->GetTimeSeconds() - StartTime;
	const float Alpha = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);

	float CurveAlpha = Alpha;
	if (SpeedCurve)
	{
		CurveAlpha = SpeedCurve->GetFloatValue(Alpha);
	}

	const FVector NewLocation =
		StartLocation + MoveDirection * (TotalDistance * CurveAlpha);

	Avatar->SetActorLocation(NewLocation, true);

	if (Target)
	{
		if (FVector::DistSquared2D(NewLocation, Target->GetActorLocation()) <= StopDistance)
		{
			FinishTask(true);
			return;
		}
	}

	if (Alpha >= 1.f)
	{
		FinishTask(false);
	}
}

void USFAbilityTask_MoveToTargetAndCheckDistance::FinishTask(bool bSuccess)
{
	if (bFinished)
		return;

	bFinished = true;
	bReachedTarget = bSuccess;

	OnFinished.Broadcast(bReachedTarget);
	EndTask();
}



void USFAbilityTask_MoveToTargetAndCheckDistance::OnDestroy(bool bInOwnerFinished)
{
	Super::OnDestroy(bInOwnerFinished);
}
