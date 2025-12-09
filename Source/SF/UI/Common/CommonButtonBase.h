#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommonButtonBase.generated.h"

class UButton;
class UTextBlock;
class USoundBase;
class UWidgetAnimation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnButtonClickedDelegate);

UCLASS()
class SF_API UCommonButtonBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	// 버튼 위에 띄울 글자 (에디터 상 수정 가능)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Common")
	FText ButtonTitle;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Clickable;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Title;
	

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_HoverGlow;

	// --------------- [Sound] ---------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Sound")
	TObjectPtr<USoundBase> HoverSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Sound")
	TObjectPtr<USoundBase> ClickSound;

public:
	UFUNCTION()
	void OnButtonClicked(); // 버튼 클릭시 이벤트

	UFUNCTION()
	void OnButtonHovered(); // 마우스를 버튼에 올렸을 때

	UFUNCTION()
	void OnButtonUnHovered(); // 마우스가 버튼을 벗어났을 때
	
public: 
	UPROPERTY(BlueprintAssignable, Category = "UI|Event")
	FOnButtonClickedDelegate OnButtonClickedDelegate;
};
