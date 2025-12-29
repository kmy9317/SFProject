// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossArenaTrigger.generated.h"

class ASFCharacterBase;
class USFDragonCombatComponent;

UCLASS()
class SF_API ABossArenaTrigger : public AActor
{
	GENERATED_BODY()

public:

	ABossArenaTrigger();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPlayerEnterArena(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<class UBoxComponent> TriggerBox;

	
	UPROPERTY(EditInstanceOnly, Category = "Boss")
	TObjectPtr<ASFCharacterBase> BossActor;

	
	UPROPERTY(Transient)
	TObjectPtr<USFDragonCombatComponent> CachedDragonCombatComponent;

	
	UPROPERTY(EditAnywhere, Category = "Boss", meta = (ClampMin = "0.0"))
	float InitialThreatValue = 100.f;

	
	UPROPERTY(Transient)
	TSet<TObjectPtr<AActor>> PlayersInArena;
};
