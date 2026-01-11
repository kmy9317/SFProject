// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Upgrade/SFSoulCountWidget.h"
#include "Components/TextBlock.h"
#include "Player/SFPlayerState.h"


void USFSoulCountWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	UpdateSoulDisplay();

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ASFPlayerState* PS = PC->GetPlayerState<ASFPlayerState>())
		{
			if (!PS->OnPlayerGoldChanged.IsAlreadyBound(this, &USFSoulCountWidget::OnPlayerGoldChanged))
			{
				PS->OnPlayerGoldChanged.AddDynamic(this, &USFSoulCountWidget::OnPlayerGoldChanged);
			}
		}
	}
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

void USFSoulCountWidget::OnPlayerGoldChanged(int32 NewGold, int32 OldGold)
{
	if (Text_SoulCount)
	{
		Text_SoulCount->SetText(FText::AsNumber(NewGold));
	}
}
