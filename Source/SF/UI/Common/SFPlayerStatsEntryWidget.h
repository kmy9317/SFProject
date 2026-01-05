#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFPlayerStatsEntryWidget.generated.h"

class UImage;
class UTextBlock;
struct FSFPlayerGameStats;
/**
 * GameOver 통계창의 플레이어별 엔트리 위젯
 */
UCLASS()
class SF_API USFPlayerStatsEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI|Stats")
	void SetPlayerStats(const FSFPlayerGameStats& Stats);

protected:
	// 플레이어 이름
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;

	// 영웅 아이콘
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> HeroIconImage;

	// 총 데미지
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DamageText;

	// 쓰러진 횟수
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DownedCountText;

	// 부활시킨 횟수
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ReviveCountText;
};
