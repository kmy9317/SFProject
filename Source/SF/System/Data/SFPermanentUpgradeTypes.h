// SFPermanentUpgradeTypes.h
#pragma once

#include "CoreMinimal.h"
#include "SFPermanentUpgradeTypes.generated.h"

// 업그레이드 카테고리
UENUM(BlueprintType)
enum class ESFUpgradeCategory : uint8
{
	Wrath UMETA(DisplayName = "Wrath[분노]"),
	Pride UMETA(DisplayName = "Pride[교만]"),
	Lust  UMETA(DisplayName = "Lust[색욕]"),
	Sloth UMETA(DisplayName = "Sloth[나태]"),
	Greed UMETA(DisplayName = "Greed[탐욕]"),

	MAX UMETA(Hidden)
};

// 티어 임계값
USTRUCT(BlueprintType)
struct SF_API FSFUpgradeTier
{
	GENERATED_BODY()

	static constexpr int32 TIER_1 = 10;
	static constexpr int32 TIER_2 = 20;
	static constexpr int32 TIER_3 = 30;
};

// 플레이어 영구 업그레이드 데이터 (PlayFab 저장값 → 게임 내 적용용으로 사용)
USTRUCT(BlueprintType)
struct SF_API FSFPermanentUpgradeData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Wrath = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Pride = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Lust = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Sloth = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Greed = 0;

	bool IsValid() const
	{
		return Wrath > 0
			|| Pride > 0
			|| Lust  > 0
			|| Sloth > 0
			|| Greed > 0;
	}
	
	// 카테고리별 포인트 조회
	int32 GetPoints(ESFUpgradeCategory Category) const
	{
		switch (Category)
		{
		case ESFUpgradeCategory::Wrath: return Wrath;
		case ESFUpgradeCategory::Pride: return Pride;
		case ESFUpgradeCategory::Lust:  return Lust;
		case ESFUpgradeCategory::Sloth: return Sloth;
		case ESFUpgradeCategory::Greed: return Greed;
		default: return 0;
		}
	}

	// 카테고리별 포인트 설정
	void SetPoints(ESFUpgradeCategory Category, int32 NewValue)
	{
		switch (Category)
		{
		case ESFUpgradeCategory::Wrath: Wrath = NewValue; break;
		case ESFUpgradeCategory::Pride: Pride = NewValue; break;
		case ESFUpgradeCategory::Lust:  Lust  = NewValue; break;
		case ESFUpgradeCategory::Sloth: Sloth = NewValue; break;
		case ESFUpgradeCategory::Greed: Greed = NewValue; break;
		default: break;
		}
	}

	// 포인트 추가
	void AddPoints(ESFUpgradeCategory Category, int32 Amount)
	{
		SetPoints(Category, GetPoints(Category) + Amount);
	}

	// 특정 티어 달성 여부
	bool HasReachedTier(ESFUpgradeCategory Category, int32 TierThreshold) const
	{
		return GetPoints(Category) >= TierThreshold;
	}

	// 현재 달성한 최고 티어 (0, 1, 2, 3)
	int32 GetCurrentTier(ESFUpgradeCategory Category) const
	{
		const int32 Points = GetPoints(Category);

		if (Points >= FSFUpgradeTier::TIER_3) return 3;
		if (Points >= FSFUpgradeTier::TIER_2) return 2;
		if (Points >= FSFUpgradeTier::TIER_1) return 1;

		return 0;
	}

	// 전체 리셋
	void ResetAll()
	{
		Wrath = 0;
		Pride = 0;
		Lust  = 0;
		Sloth = 0;
		Greed = 0;
	}
};
