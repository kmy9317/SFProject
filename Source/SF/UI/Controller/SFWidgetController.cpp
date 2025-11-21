#include "SFWidgetController.h"

void USFWidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WCParams)
{
	PlayerController = WCParams.PlayerController;
	TargetPlayerState = WCParams.PlayerState;
	TargetAbilitySystemComponent = WCParams.AbilitySystemComponent;
	TargetPrimarySet = WCParams.PrimarySet;
	TargetCombatSet = WCParams.CombatSet;
	GameState = WCParams.GameState;
}
