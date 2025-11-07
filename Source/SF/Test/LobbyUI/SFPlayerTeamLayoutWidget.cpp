#include "SFPlayerTeamLayoutWidget.h"

#include "SFNetStatics.h"
#include "SFPlayerTeamSlotWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Player/SFPlayerInfoTypes.h"
#include "Character/Hero/SFHeroDefinition.h"

void USFPlayerTeamLayoutWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TeamOneLayoutBox->ClearChildren();
	TeamTwoLayoutBox->ClearChildren();
	
	if (!PlayerTeamSlotWidgetClass)
	{
		return;
	}
	
	for (int i = 0; i < USFNetStatics::GetPlayerCountPerTeam() * 2; ++i)
	{
		USFPlayerTeamSlotWidget* NewSlotWidget = CreateWidget<USFPlayerTeamSlotWidget>(GetOwningPlayer(), PlayerTeamSlotWidgetClass);
		TeamSlotWidgets.Add(NewSlotWidget);
		
		UHorizontalBoxSlot* NewSlot;
		if (i < USFNetStatics::GetPlayerCountPerTeam())
		{
			NewSlot = TeamOneLayoutBox->AddChildToHorizontalBox(NewSlotWidget);
		}
		else
		{
			NewSlot = TeamTwoLayoutBox->AddChildToHorizontalBox(NewSlotWidget);
		}

		NewSlot->SetPadding(FMargin{ PlayerTeamWidgetSlotMargin });
	}
}

void USFPlayerTeamLayoutWidget::UpdatePlayerSelection(const TArray<FSFPlayerSelectionInfo>& PlayerSelections)
{
	for (USFPlayerTeamSlotWidget* SlotWidget : TeamSlotWidgets)
	{
		SlotWidget->UpdateSlot("", nullptr);
	}

	for (const FSFPlayerSelectionInfo& PlayerSelection : PlayerSelections)
	{
		if (!PlayerSelection.IsValid())
		{
			continue;
		}

		TeamSlotWidgets[PlayerSelection.GetPlayerSlot()]->UpdateSlot(PlayerSelection.GetPlayerNickname(), PlayerSelection.GetHeroDefinition());
	}
}