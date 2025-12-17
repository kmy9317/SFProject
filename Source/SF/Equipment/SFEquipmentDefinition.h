// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SFEquipmentDefinition.generated.h"

class UAnimInstance;
class USFGameplayAbility;
class USFEquipmentInstance;

// 무기의 Motion Warping 설정
USTRUCT(BlueprintType)
struct FSFWeaponWarpSettings
{
	GENERATED_BODY()

	FSFWeaponWarpSettings()
		: WarpTargetName(TEXT("AttackTarget"))
		, WarpRange(200.f)
		, RotationInterpSpeed(10.f)
		, MaxWindupTurnAngle(90.f)
	{
	}

	// Motion Warping 타겟 이름 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
	FName WarpTargetName;

	// Warp 이동 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp", meta = (ClampMin = "0.0"))
	float WarpRange;

	// 회전 보간 속도 (높을수록 빠른 반응, 낮을수록 묵직함)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp", meta = (ClampMin = "0.0"))
	float RotationInterpSpeed;

	// Windup 중 최대 회전 가능 각도 (0 = 제한 없음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float MaxWindupTurnAngle;
};

USTRUCT(BlueprintType)
struct FEquipmentAnimLayer
{
	GENERATED_BODY()

	// Animation Layer를 구현한 AnimInstance 클래스
	// 주의: LinkAnimClassLayers는 AnimInstance 클래스를 받으므로, 
	// Animation Layer Interface를 구현한 AnimInstance 클래스를 여기에 설정해야 함
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TSubclassOf<UAnimInstance> AnimLayerClass;

	// 무기 장착 시 활성화할 Tag (선택사항)
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	FGameplayTag GameplayTag;
};

// 1. 스폰할 Actor 정보 (메타데이터)
USTRUCT(BlueprintType)
struct FSFEquipmentActorToSpawn
{
	GENERATED_BODY()

	FSFEquipmentActorToSpawn()
	{}

	// 스폰될 Actor 클래스
	UPROPERTY(EditAnywhere, Category=Equipment)
	TSubclassOf<AActor> ActorToSpawn;
    
	// 부착할 소켓 이름
	UPROPERTY(EditAnywhere, Category=Equipment)
	FName AttachSocket;

	// 부착 Transform (소켓 기준 상대 좌표)
	UPROPERTY(EditAnywhere, Category=Equipment)
	FTransform AttachTransform;
};

//스폰할 Actor 목록 
USTRUCT(BlueprintType)
struct FSFEquipmentEntry
{
	GENERATED_BODY()
    
	FSFEquipmentEntry()
	{}
	//ex)양손검일 경우 
	UPROPERTY(EditAnywhere, Category=Equipment)
	TArray<FSFEquipmentActorToSpawn> ActorsToSpawn;
};

/**
 * 
 */
// 장착 장비 메타 데이터 
UCLASS()
class SF_API USFEquipmentDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	// Primary Asset Type 정의
	static FPrimaryAssetType GetEquipmentAssetType() 
	{ 
		return FPrimaryAssetType(TEXT("Equipment")); 
	}
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	// 장비 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName EquipmentName;
	//장비 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag EquipmentTag;
	//장비 슬롯
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag EquipmentSlotTag;
	//설명란?
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName EquipmentDescription;

	//스폰할 Actor 정보
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FSFEquipmentEntry EquipmentEntry; 

	UPROPERTY(EditDefaultsOnly) // 제공 어빌리티
	TArray<TSubclassOf<USFGameplayAbility>> GrantedAbilities;  

	//해당 무기 장착 시 사용할 AnimationLayer 
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAnimInstance> AnimLayerInfo;

	// 무기 고유의 기본 Motion Warping 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Motion Warping")
	FSFWeaponWarpSettings WarpSettings;
};
