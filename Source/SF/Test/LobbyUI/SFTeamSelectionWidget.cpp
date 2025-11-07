#include "SFTeamSelectionWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void USFTeamSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SelectButton->OnClicked.AddDynamic(this, &ThisClass::SelectButtonClicked);
}

void USFTeamSelectionWidget::SetSlotID(uint8 NewSlotID)
{
	SlotID = NewSlotID;
}

void USFTeamSelectionWidget::UpdateSlotInfo(const FString& PlayerNickname)
{
	InfoText->SetText(FText::FromString(PlayerNickname));
}

void USFTeamSelectionWidget::SelectButtonClicked()
{
	OnSlotClicked.Broadcast(SlotID);
}
