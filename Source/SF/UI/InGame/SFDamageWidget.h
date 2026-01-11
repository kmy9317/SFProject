#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFDamageWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageWidgetFinished, UUserWidget*, Widget);

UCLASS()
class SF_API USFDamageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// 데미지 효과 재생
	void PlayDamageEffect(float DamageAmount, bool bIsCritical);

	// 델리게이트 (서브시스템에서 바인딩)
	FOnDamageWidgetFinished OnFinished;

protected:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_DamageText;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	class UWidgetAnimation* Anim_PopUp;

private:
	// 애니메이션 종료 감지용 타이머 핸들
	FTimerHandle TimerHandle_ReturnToPool;

	// 타이머 콜백 함수
	void OnReturnTimerElapsed();
};