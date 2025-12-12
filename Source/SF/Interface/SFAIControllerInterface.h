// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SFAIControllerInterface.generated.h"

class USFEnemyCombatComponent;
// This class does not need to be modified.
UINTERFACE()
class USFAIControllerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SF_API ISFAIControllerInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// AIController 초기화
	virtual void InitializeAIController() = 0;
	
	virtual USFEnemyCombatComponent* GetCombatComponent() const = 0;
	
};
