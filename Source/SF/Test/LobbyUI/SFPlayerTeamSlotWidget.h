#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFPlayerTeamSlotWidget.generated.h"

class USFHeroDefinition;
class UWidgetAnimation;
class UImage;
class UTextBlock;

/**
 * 
 */
UCLASS()
class SF_API USFPlayerTeamSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	void UpdateSlot(const FString& PlayerName, const USFHeroDefinition* HeroDefinition);

	virtual void NativeOnMouseEnter( const FGeometry& InGeometry, const FPointerEvent& InMouseEvent ) override;
	virtual void NativeOnMouseLeave( const FPointerEvent& InMouseEvent ) override;

private:
	void UpdateNameText();

private:	
	UPROPERTY(Transient, meta=(BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> HoverAnim;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> PlayerHeroIcon;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(EditDefaultsOnly, Category = "SF|MaterialParams")
	FName HeroIconMatParamName = "Icon";

	UPROPERTY(EditDefaultsOnly, Category = "SF|MaterialParams")
	FName HeroEmptyMatParamName = "Empty";

	FString CachedPlayerNameStr;
	FString CachedHeroNameStr;
};
