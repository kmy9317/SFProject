// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SFEquipmentDefinition.generated.h"

class UAnimLayerInterface;
class USFGameplayAbility;
class USFEquipmentInstance;

USTRUCT(BlueprintType)
struct FEquipmentAnimLayer
{
	GENERATED_BODY()

	// Animation Layer 적용
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAnimLayerInterface> AnimLayer;

	// 무기 장착 시 활성화할 Tag (Weapon.Rifle)
	UPROPERTY(EditDefaultsOnly)
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
	FEquipmentAnimLayer AnimLayerInfo;
	
	
};
