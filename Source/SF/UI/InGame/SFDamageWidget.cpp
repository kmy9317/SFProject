// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InGame/SFDamageWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Animation/WidgetAnimation.h"


void USFDamageWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void USFDamageWidget::PlayDamageEffect(float DamageAmount)
{
	
	if (Txt_DamageText)
	{
		Txt_DamageText->SetText(FText::AsNumber(FMath::RoundToInt(DamageAmount)));
	}
	
	if (Anim_PopUp)
	{
		PlayAnimation(Anim_PopUp);
	}
}

void USFDamageWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);
	
	if (Animation == Anim_PopUp)
	{
		OnFinished.Broadcast(this);
	}
}