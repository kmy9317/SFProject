// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InGame/StagePrintWidget.h"

#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"

void UStagePrintWidget::ShowStageAnnouncement(const FText& StageName)
{
	// 1. 텍스트 설정
	if (Text_StageName)
	{
		Text_StageName->SetText(StageName);
        
		// 혹시 모르니 보이게 설정 (애니메이션이 Opacity를 조절하므로 Visible이어야 함)
		Text_StageName->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// 2. 애니메이션 재생
	if (Anim_FadeInOut)
	{
		FWidgetAnimationDynamicEvent EndEvent;
		EndEvent.BindDynamic(this, &UStagePrintWidget::OnAnnounceFinished);
		BindToAnimationFinished(Anim_FadeInOut, EndEvent);
		
		PlayAnimation(Anim_FadeInOut, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
	}
	else
	{
		// 애니메이션이 없으면 바로 제거 (방어 코드)
		RemoveFromParent();
	}
}

void UStagePrintWidget::OnAnnounceFinished()
{
	RemoveFromParent();
}
