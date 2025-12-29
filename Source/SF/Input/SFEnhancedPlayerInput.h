#pragma once

#include "CoreMinimal.h"
#include "EnhancedPlayerInput.h"
#include "SFEnhancedPlayerInput.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFEnhancedPlayerInput : public UEnhancedPlayerInput
{
	GENERATED_BODY()

public:
	USFEnhancedPlayerInput();

public:
	void FlushPressedInput(UInputAction* InputAction);
	FKey GetKeyForAction(UInputAction* InputAction) const;
};
