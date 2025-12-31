#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SFStageInfo.generated.h"

UENUM(BlueprintType)
enum class ESFLevelType : uint8
{
	Menu      UMETA(DisplayName = "Menu"),      // 메인메뉴 - 최소 에셋
	Lobby     UMETA(DisplayName = "Lobby"),     // 로비 - Lobby 번들
	InGame    UMETA(DisplayName = "InGame")     // 인게임 - InGame 번들
};

UENUM(BlueprintType)
enum class ESFStageType : uint8
{
	Normal    UMETA(DisplayName = "Normal"),
	Boss      UMETA(DisplayName = "Boss"),
	Rest      UMETA(DisplayName = "Rest")
};

USTRUCT(BlueprintType)
struct SF_API FSFStageInfo
{
	GENERATED_BODY()

	// 메인 스테이지 인덱스 (0, 1, 2 = Stage 1, 2, 3)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StageIndex = 0;

	// 서브 스테이지 인덱스 (0, 1, 2 = x-1, x-2, x-3)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SubStageIndex = 0;

	// 스테이지 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESFStageType StageType = ESFStageType::Normal;

	bool IsBossStage() const { return StageType == ESFStageType::Boss; }
	bool IsNormalStage() const { return StageType == ESFStageType::Normal; }
	bool IsRestStage() const { return StageType == ESFStageType::Rest; }

	bool operator==(const FSFStageInfo& Other) const
	{
		return StageIndex == Other.StageIndex 
			&& SubStageIndex == Other.SubStageIndex 
			&& StageType == Other.StageType;
	}
};

/**
 * 레벨별 스테이지 설정 (DataTable Row)
 */
USTRUCT(BlueprintType)
struct FSFStageConfig : public FTableRowBase
{
	GENERATED_BODY()

	// 레벨 타입 (번들 로딩용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	ESFLevelType LevelType = ESFLevelType::InGame;

	// 스테이지 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	FSFStageInfo StageInfo;

	// 스테이지 표시 이름 (UI용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	FText DisplayName;
	
};
