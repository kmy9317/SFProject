// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Upgrade/SFSoulCountWidget.h"
#include "Components/TextBlock.h"
#include "Player/SFPlayerState.h"


void USFSoulCountWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	UpdateSoulDisplay();
}

void USFSoulCountWidget::UpdateSoulDisplay()
{
	if (!Text_SoulCount) return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;
	
	if (ASFPlayerState* PS = PC->GetPlayerState<ASFPlayerState>())
	{
		int32 CurrentSoul = PS->GetGold();
		Text_SoulCount->SetText(FText::AsNumber(CurrentSoul));
	}
}
