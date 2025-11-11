#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SFMenuPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class SF_API ASFMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void OnRep_PlayerState() override;

	UUserWidget* GetMenuWidget() const { return MenuWidget; }

private:
	void SpawnWidget();
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "SF|Menu")
	TSubclassOf<UUserWidget> MenuWidgetClass;

	UPROPERTY()
	UUserWidget* MenuWidget;
};
