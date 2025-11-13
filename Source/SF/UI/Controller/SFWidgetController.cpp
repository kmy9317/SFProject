#include "SFWidgetController.h"

void USFWidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WCParams)
{
	PlayerController = WCParams.PlayerController;
	PlayerState = WCParams.PlayerState;
	AbilitySystemComponent = WCParams.AbilitySystemComponent;
	PrimarySet = WCParams.PrimarySet;
	CombatSet = WCParams.CombatSet;
	GameState = WCParams.GameState;
}
