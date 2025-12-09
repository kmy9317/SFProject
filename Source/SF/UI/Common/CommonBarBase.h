#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommonBarBase.generated.h"

class UProgressBar;
class USizeBox;
struct FLinearColor;

UCLASS()
class SF_API UCommonBarBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;
	virtual void NativePreConstruct() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Common")
	float TargetPercent;

	// 보간될 프로그래스바(Fill / Ghost) 판단을 위한 bool 변수 2종류
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Common")
	bool bIsRecovering;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Common")
	bool bIsDecreasing;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Common")
	float InterpSpeedToRecover = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Common")
	float InterpSpeedToDecrease = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Common")
	FLinearColor BarFillColor;

	// BindWidgetOptional 붙이면, WBP에 이 위젯이 없을 경우 에러없이 nullptr로 들어감.
	UPROPERTY(meta =(BindWidgetOptional))
	USizeBox* DynamicSizeBox;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Current; 

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Delayed;

public:
	// 바의 최소 길이 (너무 작아서 이미지가 찌그러지는 것 방지)
	UPROPERTY(EditAnywhere, Category = "UI|CommonBar")
	float MinBarWidth = 100.0f;
	
	// 바의 최대 길이 (화면 밖으로 나가는 것 방지)
	UPROPERTY(EditAnywhere, Category = "UI|CommonBar")
	float MaxBarWidth = 600.0f;

	// 스탯 1당 픽셀 비율
	UPROPERTY(EditAnywhere, Category = "UI|CommonBar")
	float WidthPerStat = 1.0f;
	
	// InGameHUD 호출용 함수 (0.0 ~ 1.0 사이의 수치로 UI 프로그래스바 수치 조절)
	UFUNCTION(BlueprintCallable, Category="UI|Function")
	void SetPercentVisuals(float InPercent);

	// 외부(Component)에서 색상을 바꿀 수 있게 해주는 함수
	void SetBarColor(FLinearColor NewColor);

	// 현재 능력치에 따른 길이 조절 함수
	UFUNCTION(BlueprintCallable, Category="UI|Function")
	void UpdateBarWidth(float MaxStat);
};
