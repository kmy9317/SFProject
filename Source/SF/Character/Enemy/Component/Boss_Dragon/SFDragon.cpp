// Fill out your copyright notice in the Description page of Project Settings.

#include "SFDragon.h"

#include "SFDragonGameplayTags.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/Enemy/Component/SFDragonMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Physics/SFCollisionChannels.h"


// Sets default values
ASFDragon::ASFDragon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetUseCCD(true); 
		MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		MeshComp->bEnableUpdateRateOptimizations = false; 
	}
	
}

void ASFDragon::InitializeComponents()
{
	Super::InitializeComponents();
}

void ASFDragon::OnAbilitySystemInitialized()
{
	Super::OnAbilitySystemInitialized();

	
}
