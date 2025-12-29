#include "SFTurnInPlaceComponent.h"
#include "SFBaseAIController.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFTurnInPlaceComponent)

USFTurnInPlaceComponent::USFTurnInPlaceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	SetComponentTickEnabled(true);
}

void USFTurnInPlaceComponent::BeginPlay()
{
	Super::BeginPlay();

	CooldownRemaining = 0.f;
	bIsTurning = false;
	LockedDeltaYaw = 0.f;
}

void USFTurnInPlaceComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CooldownRemaining > 0.f)
	{
		CooldownRemaining = FMath::Max(0.f, CooldownRemaining - DeltaTime);
	}

	TryTurnInPlace();
}

ASFBaseAIController* USFTurnInPlaceComponent::GetAIController() const
{
	return Cast<ASFBaseAIController>(GetOwner());
}

APawn* USFTurnInPlaceComponent::GetControlledPawn() const
{
	if (AController* Controller = Cast<AController>(GetOwner()))
	{
		return Controller->GetPawn();
	}
	return nullptr;
}

void USFTurnInPlaceComponent::TryTurnInPlace()
{
	// 이미 턴 중 or 쿨다운 중
	if (bIsTurning || CooldownRemaining > 0.f)
		return;

	APawn* Pawn = GetControlledPawn();
	if (!Pawn || !Pawn->HasAuthority())
		return;

	ASFBaseAIController* AI = GetAIController();
	if (!AI)
		return;

	// ControllerYaw 모드에서만 허용
	if (AI->GetCurrentRotationMode() != EAIRotationMode::ControllerYaw)
		return;

	// Controller 정책 체크
	if (!AI->CanTurnInPlace())
		return;

	// 이동 중이면 무시
	if (Pawn->GetVelocity().Size2D() > 10.f)
		return;

	// 각도 계산 
	const float CurrentYaw = Pawn->GetActorRotation().Yaw;
	const float TargetYaw = AI->GetControlRotation().Yaw;
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw);
	const float AbsDeltaYaw = FMath::Abs(DeltaYaw);

	if (AbsDeltaYaw < TurnThreshold)
	{
		EnableNaturalRotation(true);
		return;
	}
	
	EnableNaturalRotation(false);
	ExecuteTurn(DeltaYaw);
	CooldownRemaining = CooldownSeconds;
}

void USFTurnInPlaceComponent::ExecuteTurn(float DeltaYaw)
{
	ASFBaseAIController* AI = GetAIController();
	APawn* Pawn = GetControlledPawn();
	if (!AI || !Pawn)
		return;

	bIsTurning = true;
	LockedDeltaYaw = DeltaYaw;
	
	AI->SetbSuppressControlRotationUpdates(true);
	
	const float AbsDeltaYaw = FMath::Abs(LockedDeltaYaw);

	FGameplayTag EventTag;
	if (AbsDeltaYaw >= LargeTurnThreshold)
	{
		EventTag = (LockedDeltaYaw > 0.f)
			? SFGameplayTags::GameplayEvent_Turn_180R
			: SFGameplayTags::GameplayEvent_Turn_180L;
	}
	else
	{
		EventTag = (LockedDeltaYaw > 0.f)
			? SFGameplayTags::GameplayEvent_Turn_90R
			: SFGameplayTags::GameplayEvent_Turn_90L;
	}
	
	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = Pawn;
	Payload.EventMagnitude = LockedDeltaYaw;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Pawn,
		EventTag,
		Payload
	);

	AI->DisableTurnInPlaceFor(CooldownSeconds);
}

void USFTurnInPlaceComponent::OnTurnFinished()
{
    bIsTurning = false;
    LockedDeltaYaw = 0.f;

    if (ASFBaseAIController* AI = GetAIController())
    {
        // 회전 업데이트 재개
        AI->SetbSuppressControlRotationUpdates(false);

        // ControllerYaw 모드 유지, Focus만 재설정
        if (AI->TargetActor)
        {
            AI->SetFocus(AI->TargetActor, EAIFocusPriority::Gameplay);
        }

        AI->DisableTurnInPlaceFor(0.25f);

        // TurnInPlace 완료 후 자연 회전 재활성화
        EnableNaturalRotation(true);
    }
}

void USFTurnInPlaceComponent::EnableNaturalRotation(bool bEnable)
{
	APawn* Pawn = GetControlledPawn();
	if (!Pawn)
		return;

	ACharacter* Char = Cast<ACharacter>(Pawn);
	if (!Char)
		return;

	UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement();
	if (!MoveComp)
		return;

	ASFBaseAIController* AIC = GetAIController();
	if (!AIC)
		return;

	if (bEnable)
	{
		// 작은 각도: 자연스러운 회전 허용
		MoveComp->bUseControllerDesiredRotation = true;
		MoveComp->RotationRate = FRotator(0.f, AIC->StationaryRotationRate, 0.f); 
	}
	else
	{
		// 큰 각도: TurnInPlace 애니메이션 대기 (회전 중지)
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->RotationRate = FRotator::ZeroRotator;
	}
}