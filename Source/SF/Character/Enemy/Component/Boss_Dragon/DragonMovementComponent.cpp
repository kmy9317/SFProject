// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/Component/Boss_Dragon/DragonMovementComponent.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"
#include "MovementState/SFDragonMovementStateBase.h"
#include "MovementState/SFDragonFlyingState.h"
#include "MovementState/SFDragonGroundedState.h"
#include "MovementState/SFDragonLandingState.h"
#include "MovementState/SFDragonTakingOffState.h"
#include "MovementState/SFDragonHoveringState.h"
#include "MovementState/SFDragonDivingState.h"
#include "MovementState/SFDragonGlidingState.h"
#include "MovementState/USFDragonDisableState.h"
#include "GameFramework/Character.h"

// Sets default values for this component's properties
USFDragonMovementComponent::USFDragonMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	CurrentState = nullptr;
}

void USFDragonMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeDragonMovementComponent();
}

void USFDragonMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	UpdateGroundDistance();

	// 현재 State 업데이트
	if (CurrentState)
	{
		CurrentState->UpdateState(DeltaTime);
	}

	// 시간 누적
	TimeSinceStateChange += DeltaTime;
}

void USFDragonMovementComponent::InitializeDragonMovementComponent()
{
	
	if (AActor* Owner = GetOwner())
	{
		if (ACharacter* Character = Cast<ACharacter>(Owner))
		{
			CachedCharacterMovementComponent = Character->GetCharacterMovement();
		}
	}

	// 상태 초기화
	InitializeStateManagement();

	// 기본 상태로 시작
	SetMovementState(SFGameplayTags::Dragon_Movement_Grounded);
}


#pragma region State Management

void USFDragonMovementComponent::InitializeStateManagement()
{

	States.Add(SFGameplayTags::Dragon_Movement_Grounded,  USFDragonGroundedState::CreateState(this, this));
	States.Add(SFGameplayTags::Dragon_Movement_TakingOff, USFDragonTakingOffState::CreateState(this, this));
	States.Add(SFGameplayTags::Dragon_Movement_Flying,    USFDragonFlyingState::CreateState(this, this));
	States.Add(SFGameplayTags::Dragon_Movement_Gliding,   USFDragonGlidingState::CreateState(this, this));
	States.Add(SFGameplayTags::Dragon_Movement_Hovering,  USFDragonHoveringState::CreateState(this, this));
	States.Add(SFGameplayTags::Dragon_Movement_Diving,    USFDragonDivingState::CreateState(this, this));
	States.Add(SFGameplayTags::Dragon_Movement_Landing,   USFDragonLandingState::CreateState(this, this));
	States.Add(SFGameplayTags::Dragon_Movement_Disabled,  USFDragonDisabledState::CreateState(this, this));

	UE_LOG(LogTemp, Log, TEXT("DragonMovement: Initialized %d states"), States.Num());
}

void USFDragonMovementComponent::SetMovementState(FGameplayTag NewStateTag)
{

	if (CurrentState && CurrentState->GetType() == NewStateTag)
	{
		return;
	}


	USFDragonMovementStateBase** InState = States.Find(NewStateTag);
	if (!InState || !*InState)
	{
		return;
	}


	if (!(*InState)->CanTransitionTo())
	{
		return;
	}


	FGameplayTag OldStateTag = CurrentStateTag.IsValid() ? CurrentStateTag : SFGameplayTags::Dragon_Movement_Disabled;
	if (CurrentState)
	{
		CurrentState->ExitState();
	}

	
	CurrentState = *InState;
	CurrentStateTag = NewStateTag;
	CurrentState->EnterState();

	
	OnStateChanged.Broadcast(OldStateTag, NewStateTag);

	
	TimeSinceStateChange = 0.0f;
	
}

void USFDragonMovementComponent::StateChanged(FGameplayTag OldStateTag, FGameplayTag NewStateTag)
{
	// 상태 변경 시 추가 로직 (필요시 구현)
	UE_LOG(LogTemp, Log, TEXT("DragonMovement: StateChanged callback - %s to %s"),
		*OldStateTag.ToString(), *NewStateTag.ToString());
}

#pragma region Helper Functions

void USFDragonMovementComponent::StartFlying()
{
	
	if (CurrentStateTag == SFGameplayTags::Dragon_Movement_Grounded)
	{
		SetMovementState(SFGameplayTags::Dragon_Movement_TakingOff);
	}
	else
	{
		SetMovementState(SFGameplayTags::Dragon_Movement_Flying);
	}
}

void USFDragonMovementComponent::StartLanding()
{
	
	if (CurrentStateTag == SFGameplayTags::Dragon_Movement_Flying ||
	    CurrentStateTag == SFGameplayTags::Dragon_Movement_Hovering ||
	    CurrentStateTag == SFGameplayTags::Dragon_Movement_Gliding)
	{
		
		SetMovementState(SFGameplayTags::Dragon_Movement_Landing);
	}
}

void USFDragonMovementComponent::StartDiving()
{
	// 공중 상태에서 급강하 시작 → Diving 거침
	if (CurrentStateTag == SFGameplayTags::Dragon_Movement_Flying ||
	    CurrentStateTag == SFGameplayTags::Dragon_Movement_Hovering)
	{
		SetMovementState(SFGameplayTags::Dragon_Movement_Diving);
	}
	
}

void USFDragonMovementComponent::StartGliding()
{
	// 공중 상태에서 활공 시작
	if (CurrentStateTag == SFGameplayTags::Dragon_Movement_Flying ||
	    CurrentStateTag == SFGameplayTags::Dragon_Movement_Hovering)
	{
		SetMovementState(SFGameplayTags::Dragon_Movement_Gliding);
	}
}

void USFDragonMovementComponent::StartHovering()
{
	// Flying에서 Hovering으로 전환
	if (CurrentStateTag == SFGameplayTags::Dragon_Movement_Flying)
	{
		SetMovementState(SFGameplayTags::Dragon_Movement_Hovering);
	}
}

#pragma endregion

#pragma endregion

#pragma region Ground Distance Calculation

void USFDragonMovementComponent::UpdateGroundDistance()
{
	if (!GetOwner())
	{
		CurrentGroundDistance = 0.0f;
		return;
	}

	FVector StartLocation = GetOwner()->GetActorLocation();
	FVector EndLocation = StartLocation - FVector(0.0f, 0.0f, 10000.0f); // 아래로 10000 유닛

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	// LineTrace로 지면까지 거리 계산
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		QueryParams
	);

	if (bHit)
	{
		CurrentGroundDistance = HitResult.Distance;
	}
	else
	{
		// 지면을 찾지 못한 경우 매우 큰 값
		CurrentGroundDistance = 10000.0f;
	}
}

#pragma endregion


