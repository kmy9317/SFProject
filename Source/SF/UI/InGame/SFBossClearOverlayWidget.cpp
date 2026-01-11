// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InGame/SFBossClearOverlayWidget.h"

void USFBossClearOverlayWidget::PlayBossClearDirection()
{
	if (Anim_BossClear)
	{
		PlayAnimation(Anim_BossClear);
	}
}


void USFBossClearOverlayWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);
}
