// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "SFEquipmentBase.generated.h"

class UBoxComponent;
class UArrowComponent;

UCLASS()
class SF_API ASFEquipmentBase : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:

	ASFEquipmentBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SF|Combat")
	float WeaponBaseDamage = 10.0f;

	USkeletalMeshComponent* GetMeshComponent() const { return MeshComponent; }

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UArrowComponent> ArrowComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBoxComponent> TraceDebugCollision;
};
