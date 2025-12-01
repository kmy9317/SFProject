// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LoadingProcessInterface.h"
#include "Components/ControllerComponent.h"
#include "SFLoadingCheckComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFLoadingCheckComponent : public UControllerComponent, public ILoadingProcessInterface
{
	GENERATED_BODY()

public:
	USFLoadingCheckComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;
	
};
