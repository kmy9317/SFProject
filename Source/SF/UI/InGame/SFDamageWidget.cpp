#include "UI/InGame/SFDamageWidget.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"
#include "TimerManager.h" // 타이머 매니저 헤더 필요

void USFDamageWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void USFDamageWidget::PlayDamageEffect(float DamageAmount, bool bIsCritical)
{
	if (Txt_DamageText)
	{
		Txt_DamageText->SetText(FText::AsNumber(FMath::RoundToInt(DamageAmount)));
        
		if (bIsCritical)
		{
			Txt_DamageText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
		}
		else
		{
			Txt_DamageText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		}
	}
	
	if (Anim_PopUp)
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ReturnToPool);
		
		PlayAnimation(Anim_PopUp);
		
		float AnimDuration = Anim_PopUp->GetEndTime();
		
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ReturnToPool, this, &USFDamageWidget::OnReturnTimerElapsed, AnimDuration + 0.1f, false);
	}
	else
	{

		OnFinished.Broadcast(this);
	}
}

void USFDamageWidget::OnReturnTimerElapsed()
{
	OnFinished.Broadcast(this);
}
