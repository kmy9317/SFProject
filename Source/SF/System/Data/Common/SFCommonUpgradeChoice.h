#pragma once

#include "CoreMinimal.h"
#include "SFCommonUpgradeChoice.generated.h"

class USFCommonUpgradeDefinition;
class USFCommonRarityConfig;

/** 
 * 결정된 하나의 선택지 정보 
 * (어떤 업그레이드가, 어떤 등급으로, 최종 수치가 몇인지)
 */
USTRUCT(BlueprintType)
struct FSFCommonUpgradeChoice
{
	GENERATED_BODY()
	
	FSFCommonUpgradeChoice()
	{
		UniqueId = FGuid::NewGuid();
	}

	// 어떤 업그레이드인지
	UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
	TObjectPtr<const USFCommonUpgradeDefinition> UpgradeDefinition = nullptr;

	// 결정된 등급
	UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
	TObjectPtr<const USFCommonRarityConfig> RarityConfig = nullptr;

	// 계산된 최종 수치 -> SetByCaller로 주입될 값
	UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
	float FinalMagnitude = 0.0f;

	// UI에 보여줄 동적 설명 텍스트 (예: "공격력이 15 증가합니다")
	UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
	FText DynamicDescription;
    
	// 서버 검증용 ID (선택 시 이 ID를 서버로 보냄)
	UPROPERTY()
	FGuid UniqueId;

	bool operator==(const FSFCommonUpgradeChoice& Other) const
	{
		return UniqueId == Other.UniqueId;
	}
};