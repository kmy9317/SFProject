#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "SFCommonUpgradeFragment.generated.h"

class UGameplayEffect;

UENUM(BlueprintType)
enum class ESFUpgradeDisplayType : uint8
{
	Raw,        // 그냥 수치 (공격력 +5)
	Percent,    // 퍼센트 (치명타 확률 +5%)
	PerSecond   // 초당 (마나 재생 +0.5/초)
};

/**
 * 등급별 수치 범위
 */
USTRUCT(BlueprintType)
struct FSFRarityMagnitudeRange
{
	GENERATED_BODY()

	// 적용 등급 태그 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag RarityTag;

	// 최소 수치
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MinMagnitude = 1.0f;

	// 최대 수치
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MaxMagnitude = 1.0f;
};

/**
 * 업그레이드의 개별 효과를 정의하는 기본 Fragment 클래스
 * DefaultToInstanced, EditInlineNew: 에디터 내 인스턴스 생성 지원
 */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class SF_API USFCommonUpgradeFragment : public UObject
{
	GENERATED_BODY()
};

/**
 * [Fragment] 스탯 강화
 * GAS의 GameplayEffect와 SetByCaller를 사용하여 스탯을 동적으로 상승 시킴
 */
UCLASS(DisplayName = "Fragment: Stat Boost")
class SF_API USFCommonUpgradeFragment_StatBoost : public USFCommonUpgradeFragment
{
	GENERATED_BODY()

public:
	float GetRandomMagnitudeForRarity(const FGameplayTag& RarityTag) const;
	float GetSteppedRandomValue(float Min, float Max) const;
	float GetDisplayValue(float RawValue) const;
	FText FormatDisplayValue(float RawValue) const;
public:

	// 적용할 GameplayEffect (스탯별 개별 GE)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> EffectClass;

	// SetByCaller로 넘겨줄 태그 (예: Data.Stat.AttackPower)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	FGameplayTag AttributeTag;

	// 등급별 수치 범위
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	TArray<FSFRarityMagnitudeRange> RarityMagnitudeRanges;

	// UI 표기 타입
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	ESFUpgradeDisplayType DisplayType = ESFUpgradeDisplayType::Raw;

	// PerSecond 타입일 때 재생 주기
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display", meta = (EditCondition = "DisplayType == ESFUpgradeDisplayType::PerSecond", EditConditionHides))
	float RegenTickInterval = 0.1f;

	// 소수점 자릿수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	int32 DecimalPlaces = 0;
	
};

/**
 * [Fragment] 스킬 레벨업
 * 특정 카테고리의 스킬 레벨을 올려줌
 */
UCLASS(DisplayName = "Fragment: Skill Level")
class SF_API USFCommonUpgradeFragment_SkillLevel : public USFCommonUpgradeFragment
{
	GENERATED_BODY()

public:
	// 레벨을 올릴 대상 스킬 태그 (예: InputTag.Primary)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
	FGameplayTag TargetSkillInputTag;

	// 증가시킬 레벨 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
	int32 LevelIncrement = 1;
};