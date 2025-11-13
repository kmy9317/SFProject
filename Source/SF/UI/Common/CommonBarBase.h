#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommonBarBase.generated.h"

class UProgressBar;
struct FLinearColor;

UCLASS()
class SF_API UCommonBarBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;
	virtual void NativePreConstruct() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI/Common/CommonBarBase")
	float TargetPercent;

	// 보간될 프로그래스바(Fill / Ghost) 판단을 위한 bool 변수 2종류
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI/Common/CommonBarBase")
	bool bIsRecovering;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI/Common/CommonBarBase")
	bool bIsDecreasing;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI/Common/CommonBarBase")
	float InterpSpeedToRecover = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI/Common/CommonBarBase")
	float InterpSpeedToDecrease = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI/Common/CommonBarBase")
	FLinearColor BarFillColor;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Current; 

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Delayed;
	
public:
	// InGameHUD 호출용 함수 (0.0 ~ 1.0 사이의 수치로 UI 프로그래스바 수치 조절)
	UFUNCTION(BlueprintCallable, Category="UI/Common/CommonBarBase")
	void SetPercentVisuals(float InPercent);
};
