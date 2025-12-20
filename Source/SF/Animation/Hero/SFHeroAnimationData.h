#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SFHeroAnimationData.generated.h"

/**
 * 단일 몽타주 재생 데이터
 */
USTRUCT(BlueprintType)
struct FSFMontagePlayData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float PlayRate = 1.0f;

	// 시작 섹션 (NAME_None이면 처음부터) 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName StartSection = NAME_None;

	bool IsValid() const { return Montage != nullptr; }
};

/**
 * 랜덤 몽타주 세트 (같은 동작, 다른 모션)
 */
USTRUCT(BlueprintType)
struct FSFRandomMontageData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FSFMontagePlayData> Montages;

	FSFMontagePlayData GetRandom() const
	{
		if (Montages.IsEmpty()) return FSFMontagePlayData();
		return Montages[FMath::RandRange(0, Montages.Num() - 1)];
	}

	FSFMontagePlayData GetByIndex(int32 Index) const
	{
		return Montages.IsValidIndex(Index) ? Montages[Index] : FSFMontagePlayData();
	}

	bool IsValid() const { return !Montages.IsEmpty(); }
	int32 Num() const { return Montages.Num(); }
};

/**
 * 방향별 몽타주 세트 (피격, 밀려나기 등)
 */
USTRUCT(BlueprintType)
struct FSFDirectionalMontageData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FSFRandomMontageData Front;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FSFRandomMontageData Back;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FSFRandomMontageData Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FSFRandomMontageData Right;
	
	// 각도로 방향별 몽타주 조회 (랜덤), 캐릭터 기준 각도 (-180 ~ 180, 0 = 정면)
	FSFMontagePlayData GetByDirection(float AngleDegrees) const;

	bool IsValid() const { return Front.IsValid() || Back.IsValid() || Left.IsValid() || Right.IsValid(); }
};


/**
 * Hero 캐릭터의 애니메이션 데이터
 * - PawnData에서 참조
 * - 스켈레톤별로 별도 에셋 생성
 */
UCLASS()
class SF_API USFHeroAnimationData : public UDataAsset
{
	GENERATED_BODY()

public:
	
	// 상황에 대한 단일 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TMap<FGameplayTag, FSFMontagePlayData> SingleMontages;

	// 상황에 대한 랜덤 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TMap<FGameplayTag, FSFRandomMontageData> RandomMontages;

	// 방향별 몽타주 (피격 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TMap<FGameplayTag, FSFDirectionalMontageData> DirectionalMontages;

	// 단일 몽타주 조회 
	UFUNCTION(BlueprintCallable, Category = "Animation")
	FSFMontagePlayData GetSingleMontage(const FGameplayTag& Tag) const;

	// 랜덤 몽타주 조회 
	UFUNCTION(BlueprintCallable, Category = "Animation")
	FSFMontagePlayData GetRandomMontage(const FGameplayTag& Tag) const;
	
	// 방향별 몽타주 조회
	UFUNCTION(BlueprintCallable, Category = "Animation")
	FSFMontagePlayData GetDirectionalMontage(const FGameplayTag& Tag, float AngleDegrees) const;

protected:
	// 부모 태그 Fallback 포함 조회 
	template<typename T>
	const T* FindByTag(const TMap<FGameplayTag, T>& Map, const FGameplayTag& Tag) const
	{
		if (!Tag.IsValid())
		{
			return nullptr;
		}

		if (const T* Found = Map.Find(Tag))
		{
			return Found;
		}

		for (const auto& Pair : Map)
		{
			if (Tag.MatchesTag(Pair.Key))
			{
				return &Pair.Value;
			}
		}

		return nullptr;
	}
};
