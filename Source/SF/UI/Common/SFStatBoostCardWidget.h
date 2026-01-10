#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFStatBoostCardWidget.generated.h"

class UOverlay;
struct FSFCommonUpgradeChoice;
class USFCommonRarityConfig;
class UTextBlock;
class UImage;
class UBorder;
class UButton;
class UWidgetAnimation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatBoostCardSelected, int32, CardIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCardAnimationFinished);

/**
 * 일반 강화 개별 카드 위젯
 */
UCLASS()
class SF_API USFStatBoostCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 카드 데이터 설정 
	UFUNCTION(BlueprintCallable, Category = "SF|UI")
	void SetCardData(const FSFCommonUpgradeChoice& Choice, int32 InCardIndex);

	void ResetCardVisuals();

	UFUNCTION(BlueprintCallable, Category = "SF|UI")
	void EnableButtonWithDelay();

	// 버튼 활성화/비활성화 
	UFUNCTION(BlueprintCallable, Category = "SF|UI")
	void SetButtonEnabled(bool bEnabled);

	// 카드 공개 애니메이션 (BP에서 구현) 
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SF|UI|Animation")
	void PlayCardReveal();

	// 선택 애니메이션 완료 알림 (BP에서 호출) 
	UFUNCTION(BlueprintCallable, Category = "SF|UI")
	void NotifyAnimationComplete();

protected:
	// 등급에 따른 비주얼 적용 
	UFUNCTION(BlueprintCallable, Category = "SF|UI")
	void ApplyRarityVisuals(const USFCommonRarityConfig* RarityConfig);

private:
	UFUNCTION()
	void OnCardClicked();

public:
	UPROPERTY(BlueprintAssignable, Category = "SF|UI|Event")
	FOnStatBoostCardSelected OnCardSelectedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "SF|UI|Event")
	FOnCardAnimationFinished OnAnimationFinishedDelegate;

protected:
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_Entire;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_Front;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Title;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Desc;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_RarityFrame;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_Background;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Select;

	// 버튼 활성화 딜레이 (초) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SF|UI|Settings")
	float ButtonEnableDelay = 1.0f;

private:
	int32 CurrentCardIndex = INDEX_NONE;

	FTimerHandle ButtonEnableTimerHandle;
};
