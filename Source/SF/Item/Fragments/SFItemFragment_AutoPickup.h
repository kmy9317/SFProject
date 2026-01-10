#pragma once

#include "CoreMinimal.h"
#include "Item/SFItemDefinition.h"
#include "SFItemFragment_AutoPickup.generated.h"

class ASFAutoPickup;

/**
 * 자동 습득 아이템 Fragment
 * 플레이어가 범위 내에 들어오면 자석처럼 끌려와서 자동 수집됨
 */
UCLASS()
class SF_API USFItemFragment_AutoPickup : public USFItemFragment
{
	GENERATED_BODY()

public:
	// 스폰할 액터 클래스 (BP_AutoPickup_Gold 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutoPickup")
	TSubclassOf<ASFAutoPickup> PickupActorClass;

	// 자석 감지 범위
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutoPickup")
	float DetectionRadius = 500.f;

	// 수집 범위
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutoPickup")
	float CollectionRadius = 50.f;

	// Homing 최대 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutoPickup")
	float HomingSpeed = 1500.f;

	// Homing 가속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutoPickup")
	float HomingAcceleration = 3000.f;

	// 스폰 후 자석 활성화까지 딜레이
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutoPickup")
	float InitialDelay = 0.3f;

	// 모든 플레이어에게 효과 적용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutoPickup")
	bool bApplyToAllPlayers = true;
};
