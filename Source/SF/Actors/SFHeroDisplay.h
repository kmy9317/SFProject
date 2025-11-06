#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SFHeroDisplay.generated.h"

class USFHeroDefinition;
class UCameraComponent;

UCLASS()
class SF_API ASFHeroDisplay : public AActor
{
	GENERATED_BODY()

public:
	ASFHeroDisplay();
	void ConfigureWithHeroDefination(const USFHeroDefinition* HeroDefinition);

private:
	UPROPERTY(VisibleDefaultsOnly, Category = "Character Display")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "Character Display")
	TObjectPtr<UCameraComponent> ViewCameraComponent;
};
