// Fill out your copyright notice in the Description page of Project Settings.


#include "SFDragon.h"

#include "DragonMovementComponent.h"
#include "Components/CapsuleComponent.h"


// Sets default values
ASFDragon::ASFDragon()
{
	//CapsuleComponent 이건 오직 MovementComponent를 위해 존재
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	//모든 충돌 Collision은 메시 컴포넌트로
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::Type::QueryOnly);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	
	DragonMovementComponent = CreateDefaultSubobject<USFDragonMovementComponent>(TEXT("DragonMovementComponent"));
	DragonMovementComponent->SetIsReplicated(true);
	
}

void ASFDragon::InitializeMovementComponent()
{
	Super::InitializeMovementComponent();

	if (IsValid(DragonMovementComponent))
	{
		DragonMovementComponent->InitializeDragonMovementComponent();
	}
}





