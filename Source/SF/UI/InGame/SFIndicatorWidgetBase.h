// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFIndicatorWidgetBase.generated.h"

class UImage;
class UTextBlock;
class UWidget;
class UCanvasPanelSlot;

/**
 * 팀원 위치 표시기용 베이스 위젯
 */

UCLASS()
class SF_API USFIndicatorWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	// 타겟 설정 함수
	void SetTargetActor(AActor* InTargetActor);
	// 플레이어 이름 업데이트 함수
	void UpdatePlayerName();
	// 위젯 위치 및 회전 함수
	void UpdateIndicatorTransform();
	// 타겟(Actor)이 메모리에서 사라졌는지 확인하는 헬퍼 함수
	bool HasValidTarget() const { return TargetActorPtr.IsValid(); }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	// 추적 대상 (약한 참조)
	TWeakObjectPtr<AActor> TargetActorPtr;

	// 화면 가장자리 여백 (픽셀)
	UPROPERTY(EditDefaultsOnly, Category = "UI|InGame")
	float ScreenEdgeMargin = 150.0f;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> IndicatorRoot; // 위치를 이동시킬 부모 컨테이너

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ArrowImage;     // 화살표 이미지

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;   // 이름 텍스트

private:
	// 성능 최적화를 위해 슬롯을 캐싱
	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> RootCanvasSlot;
	

};
