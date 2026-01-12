// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interface/SFMiniMapTrackable.h"
#include "Subsystems/WorldSubsystem.h"
#include "SFMinimapSubsystem.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMiniMapTargetChanged, TScriptInterface<ISFMiniMapTrackable>, Target);

/**
 * 
 */
UCLASS()
class SF_API USFMinimapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 등록/해제
	UFUNCTION(BlueprintCallable, Category = "MiniMap")
	void RegisterTarget(TScriptInterface<ISFMiniMapTrackable> Target);

	UFUNCTION(BlueprintCallable, Category = "MiniMap")
	void UnregisterTarget(TScriptInterface<ISFMiniMapTrackable> Target);
	
	UFUNCTION(BlueprintCallable, Category = "MiniMap")
	const TArray<TScriptInterface<ISFMiniMapTrackable>>& GetAllTargets() const;
	
	UFUNCTION(BlueprintCallable, Category = "MiniMap")
	TArray<TScriptInterface<ISFMiniMapTrackable>> GetTargetsByType(EMiniMapIconType Type) const;
	
	UPROPERTY(BlueprintAssignable, Category = "MiniMap")
	FOnMiniMapTargetChanged OnTargetRegistered;

	UPROPERTY(BlueprintAssignable, Category = "MiniMap")
	FOnMiniMapTargetChanged OnTargetUnregistered;
	
private:
	UPROPERTY()
	TArray<TScriptInterface<ISFMiniMapTrackable>> Targets;
};
