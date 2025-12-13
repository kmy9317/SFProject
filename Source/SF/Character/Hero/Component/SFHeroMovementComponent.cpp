#include "SFHeroMovementComponent.h"


void USFHeroMovementComponent::SetSlidingMode(ESFSlidingMode NewMode)
{
	// 이전 모드가 PassThrough였으면 충돌 복원
	if (SlidingMode == ESFSlidingMode::PassThrough && NewMode != ESFSlidingMode::PassThrough)
	{
		RestoreCollision();
	}

	SlidingMode = NewMode;

	// 새 모드가 PassThrough면 충돌 변경
	if (NewMode == ESFSlidingMode::PassThrough)
	{
		ApplyPassThroughCollision();
	}
}

float USFHeroMovementComponent::SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact)
{
	switch (SlidingMode)
	{
	case ESFSlidingMode::StopOnHit:
		{
			// 이동 방향과 충돌면 법선 비교
			const FVector MoveDir = Delta.GetSafeNormal();
			const float DotProduct = FVector::DotProduct(MoveDir, -Normal);

			// 정면 충돌일 때만 정지
			if (DotProduct > FrontalHitThreshold)
			{
				return 0.f;
			}

			// 측면 스침은 기본 슬라이딩 허용
			return Super::SlideAlongSurface(Delta, Time, Normal, Hit, bHandleImpact);
		}

	case ESFSlidingMode::PassThrough:
	case ESFSlidingMode::Normal:
	default:
		return Super::SlideAlongSurface(Delta, Time, Normal, Hit, bHandleImpact);
	}
}

void USFHeroMovementComponent::ApplyPassThroughCollision()
{
	if (!UpdatedPrimitive)
	{
		return;
	}

	SavedCollisionResponses.Empty();

	for (ECollisionChannel Channel : PassThroughChannels)
	{
		SavedCollisionResponses.Add(Channel, UpdatedPrimitive->GetCollisionResponseToChannel(Channel));
		UpdatedPrimitive->SetCollisionResponseToChannel(Channel, ECR_Overlap);
	}
}

void USFHeroMovementComponent::RestoreCollision()
{
	if (!UpdatedPrimitive)
	{
		return;
	}

	for (const auto& Pair : SavedCollisionResponses)
	{
		UpdatedPrimitive->SetCollisionResponseToChannel(Pair.Key, Pair.Value);
	}

	SavedCollisionResponses.Empty();
}
