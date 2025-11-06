#include "SFMenuPlayerController.h"

#include "Blueprint/UserWidget.h"

void ASFMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeUIOnly());
	SetShowMouseCursor(true);

	// for listen server
	if (HasAuthority() && IsLocalPlayerController())
	{
		SpawnWidget();
	}
}

void ASFMenuPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (IsLocalPlayerController())
	{
		SpawnWidget();
	}
}

void ASFMenuPlayerController::SpawnWidget()
{
	if (MenuWidgetClass)
	{
		MenuWidget = CreateWidget<UUserWidget>(this, MenuWidgetClass);
		if (MenuWidget)
		{
			MenuWidget->AddToViewport();
		}
	}
}