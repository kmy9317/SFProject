// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFSoulCountWidget.generated.h"

class UTextBlock;
class USFPlayFabSubsystem;

/**
 * 
 */
UCLASS()
class SF_API USFSoulCountWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// 1. 에디터의 UMG와 연결할 텍스트 블록
	UPROPERTY(meta=(BindWidget))
	UTextBlock* Text_SoulCount = nullptr;

private:
	// 2. 데이터를 표시할 함수
	void UpdateSoulDisplay();

	// 3. 데이터 원본에 접근하기 위한 서브시스템 캐싱
	UPROPERTY()
	USFPlayFabSubsystem* Subsystem = nullptr;
	
};
