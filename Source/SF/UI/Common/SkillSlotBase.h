#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "UI/SFUserWidget.h"
#include "SkillSlotBase.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;

UCLASS()
class SF_API USkillSlotBase : public USFUserWidget
{
	GENERATED_BODY()

protected:
	
	virtual void NativeOnWidgetControllerSet() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void InitializeSlot();
	void RefreshCooldown();

	UFUNCTION()
	void OnAbilityChanged(FGameplayAbilitySpecHandle AbilitySpecHandle, bool bGiven);
	
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

	// 이 슬롯이 표시할 어빌리티의 InputTag
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "SF|SkillSlot", meta = (Categories = "InputTag"))
	FGameplayTag SlotInputTag;

	// 키 프롬프트 텍스트
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "SF|SkillSlot")
	FText KeyPromptText;

private:
	FGameplayAbilitySpecHandle CachedAbilitySpecHandle;
	float CachedCooldownDuration = 0.f;

public:
	UFUNCTION(BlueprintCallable, Category = "UI|Function")
	void SetSlotVisuals(UTexture2D* InIcon, const FText& InKeyText);

	UFUNCTION(BlueprintCallable, Category = "UI|Function")
	void StartCooldown(float InDuration);
};
