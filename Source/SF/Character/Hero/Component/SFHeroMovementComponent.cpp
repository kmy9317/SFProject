#include "SFHeroMovementComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "MotionWarpingComponent.h"
#include "AbilitySystem/Attributes/Hero/SFPrimarySet_Hero.h"
#include "GameFramework/Character.h"

void USFHeroMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		CachedMotionWarpingComp = Owner->FindComponentByClass<UMotionWarpingComponent>();
	}
}

USFHeroMovementComponent::USFHeroMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentWarpTargetLocation = FVector::ZeroVector;
	CurrentWarpTargetRotation = FRotator::ZeroRotator;
	bHasActiveWarpTarget = false;

	SetNetworkMoveDataContainer(SFNetworkMoveDataContainer);
}

float USFHeroMovementComponent::GetMaxSpeed() const
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		const UAttributeSet* AttributeSet = ASC->GetAttributeSet(USFPrimarySet_Hero::StaticClass());
		if (const USFPrimarySet_Hero* PrimarySet = Cast<USFPrimarySet_Hero>(AttributeSet))
		{
			float MoveSpeed = PrimarySet->GetMoveSpeed();
			float MoveSpeedPercent = PrimarySet->GetMoveSpeedPercent();

			// 기본 속도 + 퍼센트 보너스
			float MaxMoveSpeed = FMath::Max(0.f, MoveSpeed + MoveSpeed * (MoveSpeedPercent / 100.f));
			switch(MovementMode)
			{
			case MOVE_Walking:
			case MOVE_NavWalking:
			{
				float DirectionDot = GetOwner()->GetActorForwardVector().Dot(Velocity.GetSafeNormal());
				if (DirectionDot < 0.25f)
				{
					if (DirectionDot > -0.25f)
					{
						MaxMoveSpeed = MaxMoveSpeed * LeftRightMovePercent;
					}
					else
					{
						MaxMoveSpeed = MaxMoveSpeed * BackwardMovePercent;
					}
				}

				MaxMoveSpeed = IsCrouching() ? CrouchMovePercent * MaxMoveSpeed : MaxMoveSpeed;	
				//GEngine->AddOnScreenDebugMessage(1, 0.1f, FColor::Red, FString::Printf(TEXT("MaxSpeed: %f"), MaxMoveSpeed));
				return MaxMoveSpeed;
			}
			case MOVE_Falling:
				return MaxMoveSpeed;
			case MOVE_Swimming:
				return MaxSwimSpeed;
			case MOVE_Flying:
				return MaxFlySpeed;
			case MOVE_Custom:
				return MaxCustomMovementSpeed;
			case MOVE_None:
			default:
				return 0.f;
			}
		}
	}
	return Super::GetMaxSpeed();
}

void USFHeroMovementComponent::SetWarpTarget(const FVector& Location, const FRotator& Rotation)
{
	CurrentWarpTargetLocation = Location;
	CurrentWarpTargetRotation = Rotation;
	bHasActiveWarpTarget = true;
}

void USFHeroMovementComponent::ClearWarpTarget()
{
	CurrentWarpTargetLocation = FVector::ZeroVector;
	CurrentWarpTargetRotation = FRotator::ZeroRotator;
	bHasActiveWarpTarget = false;
}

FNetworkPredictionData_Client* USFHeroMovementComponent::GetPredictionData_Client() const
{
	// 필요할 때만 할당
	if (ClientPredictionData == nullptr)
	{
		USFHeroMovementComponent* MutableThis = const_cast<USFHeroMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_SFCharacter(*this);
	}

	return ClientPredictionData;
}

void USFHeroMovementComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel)
{
	// 서버가 클라이언트 패킷을 처리하는 진입점으로써 클라이언트로부터 받은 Warp 타겟 데이터 적용
	if (GetNetMode() != NM_Client)
	{
		const FSFCharacterNetworkMoveData* MoveData = static_cast<const FSFCharacterNetworkMoveData*>(GetCurrentNetworkMoveData());
		if (MoveData && MoveData->bNetworkHasWarpTarget)
		{
			CurrentWarpTargetLocation = MoveData->NetworkWarpTargetLocation;
			CurrentWarpTargetRotation = MoveData->NetworkWarpTargetRotation;
			bHasActiveWarpTarget = true;

			ApplyWarpTargetToMotionWarping();
		}
	}

	Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
}

void USFHeroMovementComponent::ApplyWarpTargetToMotionWarping()
{
	if (!bHasActiveWarpTarget ||!CachedMotionWarpingComp)
	{
		return;
	}

	// Task에서 설정한 WarpTargetName과 동일해야 함
	static const FName WarpTargetName = TEXT("AttackTarget");
	CachedMotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, CurrentWarpTargetLocation, CurrentWarpTargetRotation);
}

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

void FSavedMove_SFCharacter::Clear()
{
	FSavedMove_Character::Clear();

	SavedWarpTargetLocation = FVector::ZeroVector;
	SavedWarpTargetRotation = FRotator::ZeroRotator;
	bSavedHasWarpTarget = false;
}

void FSavedMove_SFCharacter::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	// 부모 클래스의 기본 데이터 저장 (위치, 속도, 회전 등)
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	if (USFHeroMovementComponent* SFMovement = Cast<USFHeroMovementComponent>(C->GetCharacterMovement()))
	{
		SavedWarpTargetLocation = SFMovement->CurrentWarpTargetLocation;
		SavedWarpTargetRotation = SFMovement->CurrentWarpTargetRotation;
		bSavedHasWarpTarget = SFMovement->bHasActiveWarpTarget;
	}
}

void FSavedMove_SFCharacter::PrepMoveFor(ACharacter* C)
{
	// 부모 클래스의 기본 복원 (위치, 속도, 회전 등)
	FSavedMove_Character::PrepMoveFor(C);

	// 재시뮬레이션 발생 시 호출(이전의 데이터를 CMC에 다시 주입하여 서버와 동일한 조건에서 재연산하도록 함)
	if (USFHeroMovementComponent* SFMovement = Cast<USFHeroMovementComponent>(C->GetCharacterMovement()))
	{
		SFMovement->CurrentWarpTargetLocation = SavedWarpTargetLocation;
		SFMovement->CurrentWarpTargetRotation = SavedWarpTargetRotation;
		SFMovement->bHasActiveWarpTarget = bSavedHasWarpTarget;

		SFMovement->ApplyWarpTargetToMotionWarping();
	}
}

bool FSavedMove_SFCharacter::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_SFCharacter* NewSFMove = static_cast<const FSavedMove_SFCharacter*>(NewMove.Get());
	
    // Warp 타겟이 다르면 병합 불가
    if (bSavedHasWarpTarget != NewSFMove->bSavedHasWarpTarget)
    {
        return false;
    }

    if (bSavedHasWarpTarget)
    {
        if (!SavedWarpTargetLocation.Equals(NewSFMove->SavedWarpTargetLocation, 1.f) ||
            !SavedWarpTargetRotation.Equals(NewSFMove->SavedWarpTargetRotation, 1.f))
        {
            return false;
        }
    }
	
	return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

FNetworkPredictionData_Client_SFCharacter::FNetworkPredictionData_Client_SFCharacter(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
	
}

FSavedMovePtr FNetworkPredictionData_Client_SFCharacter::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_SFCharacter());
}
