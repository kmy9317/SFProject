// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "SFDamageTextSubSystem.generated.h"


class USFDamageWidget;
class UWidgetComponent;
/**
 * 
 */
UCLASS()
class SF_API USFDamageTextSubSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	void ShowDamage(float DamageAmount, AActor* TargetActor, FVector HitLocation);

private:
	UWidgetComponent* GetFromPool();
	void ReturnToPool(UWidgetComponent* Component);
	FVector CalcDamageTextLocation(AActor* TargetActor, FVector HitLocation);

	UFUNCTION()
	void OnWidgetAnimationFinished(UUserWidget* Widget);

	UPROPERTY()
	TSubclassOf<USFDamageWidget> DamageWidgetClass;

	UPROPERTY()
	TObjectPtr<AActor> PoolOwnerActor;

	UPROPERTY()
	TArray<TObjectPtr<UWidgetComponent>> AvailablePool;

	UPROPERTY()
	TMap<TObjectPtr<UUserWidget>, TObjectPtr<UWidgetComponent>> ActiveWidgetMap;
};
