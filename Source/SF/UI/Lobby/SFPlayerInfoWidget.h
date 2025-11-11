// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/SFPlayerInfoTypes.h"
#include "SFPlayerInfoWidget.generated.h"

class URetainerBox;
class UTextBlock;

/**
 * HeroDisplay 위에 표시되는 플레이어 정보 위젯
 * 블루프린트에서 상속받아 비주얼 구성
 */
UCLASS()
class SF_API USFPlayerInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** PlayerInfo 업데이트  */
	UFUNCTION(BlueprintCallable, Category = "PlayerInfo")
	void UpdatePlayerInfo(const FSFPlayerInfo& NewPlayerInfo);

protected:

	/** 플레이어 이름 텍스트 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_PlayerName;

	/** Ready 상태 텍스트 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ReadyStatus;

	/** RetainerBox (Opacity 조절용) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<URetainerBox> RetainerBox;

	/** 현재 PlayerInfo */
	UPROPERTY(BlueprintReadOnly, Category = "PlayerInfo")
	FSFPlayerInfo PlayerInfo;
};
