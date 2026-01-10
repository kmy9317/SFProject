// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFDamageWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;

/**
 * 데미지 텍스트 전용 위젯 클래스
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageWidgetFinished, UUserWidget*, Widget);

UCLASS()
class SF_API USFDamageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	
	void PlayDamageEffect(float DamageAmount);

	// 애니메이션 종료 감지
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

public:
	// Subsystem이 듣게 될 이벤트
	FOnDamageWidgetFinished OnFinished;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> Txt_DamageText;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> Anim_PopUp;
};
