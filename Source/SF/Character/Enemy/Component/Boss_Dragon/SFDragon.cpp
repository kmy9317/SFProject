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

	LockOnSockets.Add(FName("Chest_M"));	// 가슴
	LockOnSockets.Add(FName("Head_M"));	    // 머리
	LockOnSockets.Add(FName("Toes1_L"));	// 왼발
	LockOnSockets.Add(FName("Toes1_R"));	// 오른발
	LockOnSockets.Add(FName("Tail10_M"));   // 꼬리
}

void ASFDragon::InitializeComponents()
{
	Super::InitializeComponents();
}

void ASFDragon::OnAbilitySystemInitialized()
{
	Super::OnAbilitySystemInitialized();

	
}

TArray<FName> ASFDragon::GetLockOnSockets() const
{
	// 설정된 소켓 리스트가 있다면 반환
	if (LockOnSockets.Num() > 0)
	{
		return LockOnSockets;
	}
	
	// 설정이 비어있다면 부모(SFEnemy)의 기본 동작(단일 소켓) 따름
	return Super::GetLockOnSockets();
}
