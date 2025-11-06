#include "SFHeroDisplay.h"

#include "Camera/CameraComponent.h"
#include "Character/Hero/SFHeroDefinition.h"


ASFHeroDisplay::ASFHeroDisplay()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root Comp"));

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("Mesh Component");
	MeshComponent->SetupAttachment(GetRootComponent());
	
	ViewCameraComponent = CreateDefaultSubobject<UCameraComponent>("View Camera Component");
	ViewCameraComponent->SetupAttachment(GetRootComponent());
}

void ASFHeroDisplay::ConfigureWithHeroDefination(const USFHeroDefinition* HeroDefination)
{
	if (!HeroDefination)
	{
		return;
	}
	
	MeshComponent->SetSkeletalMesh(HeroDefination->LoadDisplayMesh());
	MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	MeshComponent->SetAnimInstanceClass(HeroDefination->LoadDisplayAnimationBP());
}

