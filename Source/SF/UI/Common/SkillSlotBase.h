#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillSlotBase.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;

UCLASS()
class SF_API USkillSlotBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_SkillIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Cooldown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CooldownCount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_KeyPrompt;

	UPROPERTY()
	bool bIsOnCooldown;

	UPROPERTY()
	float CooldownTotalDuration;

	UPROPERTY()
	float CooldownEndTime;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	UFUNCTION(BlueprintCallable, Category = "UI/Common/SkillSlotBase")
	void SetSlotVisuals(UTexture2D* InIcon, const FText& InKeyText);

	UFUNCTION(BlueprintCallable, Category = "UI/Common/SkillSlotBase")
	void StartCooldown(float InDuration);
};
