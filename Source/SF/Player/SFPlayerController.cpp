#include "SFPlayerController.h"

#include "AbilitySystemGlobals.h"
#include "SFPlayerState.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"

ASFPlayerController::ASFPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ASFPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
}

void ASFPlayerController::OnUnPossess()
{
	// Make sure the pawn that is being unpossessed doesn't remain our ASC's avatar actor
	if (APawn* PawnBeingUnpossessed = GetPawn())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState))
		{
			if (ASC->GetAvatarActor() == PawnBeingUnpossessed)
			{
				ASC->SetAvatarActor(nullptr);
			}
		}
	}

	Super::OnUnPossess();
}

void ASFPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 클라이언트에서 PlayerController가 늦게 복제된 경우
	if (GetWorld()->IsNetMode(NM_Client))
	{
		if (ASFPlayerState* SFPS = GetPlayerState<ASFPlayerState>())
		{
			if (USFAbilitySystemComponent* SFASC = SFPS->GetSFAbilitySystemComponent())
			{
				SFASC->RefreshAbilityActorInfo();
				SFASC->TryActivateAbilitiesOnSpawn();
			}
		}
	}
}

ASFPlayerState* ASFPlayerController::GetSFPlayerState() const
{
	return CastChecked<ASFPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

USFAbilitySystemComponent* ASFPlayerController::GetSFAbilitySystemComponent() const
{
	const ASFPlayerState* LCPS = GetSFPlayerState();
	return (LCPS ? LCPS->GetSFAbilitySystemComponent() : nullptr);
}


void ASFPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent())
	{
		SFASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}
	
	Super::PostProcessInput(DeltaTime, bGamePaused);

}
