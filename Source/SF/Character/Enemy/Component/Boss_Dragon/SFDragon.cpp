// Fill out your copyright notice in the Description page of Project Settings.

#include "SFDragon.h"

#include "SFDragonGameplayTags.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/Enemy/Component/SFDragonMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Physics/SFCollisionChannels.h"


// Sets default values
ASFDragon::ASFDragon()
{
	
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn")); 

    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	
    GetCapsuleComponent()->SetCollisionResponseToChannel(SF_ObjectChannel_Weapon, ECR_Ignore);


    GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetMesh()->SetCollisionObjectType(ECC_Pawn); 
    GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);

    GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

    GetMesh()->SetCollisionResponseToChannel(SF_ObjectChannel_Weapon, ECR_Overlap);
	

    GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); 

    GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
    GetMesh()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);

    GetMesh()->SetUseCCD(true); 
    GetMesh()->SetGenerateOverlapEvents(true);
	
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	
	GetMesh()->bEnableUpdateRateOptimizations = false; 
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(SF_ObjectChannel_Weapon, ECR_Overlap);


}

void ASFDragon::InitializeComponents()
{
	Super::InitializeComponents();
}

void ASFDragon::OnAbilitySystemInitialized()
{
	Super::OnAbilitySystemInitialized();

	
}






