// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameOverScreenWidget.generated.h"

/**
 * 
 */
UCLASS()
class SF_API UGameOverScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 외부(PC)에서 호출할 함수
	UFUNCTION(BlueprintCallable, Category = "UI|GameOver")
	void PlayGameOverDirection();

protected:
	// 위젯 애니메이션 종료 시 처리
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

public:
	// Anim 바인딩
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation*	Anim_GameOVer;
};
