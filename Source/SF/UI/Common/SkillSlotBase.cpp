#include "UI/Common/SkillSlotBase.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void USkillSlotBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsOnCooldown) return;

	if (!PB_Cooldown || !Text_CooldownCount)
	{
		bIsOnCooldown = false; // 해당 Widget이 존재하지 않을경우 즉시 종료
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float TimeRemaining = CooldownEndTime - CurrentTime;

	if (TimeRemaining > 0.0f)
	{
		FString TimeString = FString::Printf(TEXT("%.1f"), TimeRemaining);
		Text_CooldownCount->SetText(FText::FromString(TimeString));

		const float Percent = TimeRemaining / CooldownTotalDuration;
		PB_Cooldown->SetPercent(Percent);
	}
	// TimeRemaining <= 0.0f 쿨타임 종료 
	else 
	{
		bIsOnCooldown = false;
		PB_Cooldown->SetVisibility(ESlateVisibility::Hidden);
		Text_CooldownCount->SetVisibility(ESlateVisibility::Hidden);
	}
	
}

void USkillSlotBase::SetSlotVisuals(UTexture2D* InIcon, const FText& InKeyText)
{
	if (Img_SkillIcon) Img_SkillIcon->SetBrushFromTexture(InIcon);
	if (Text_KeyPrompt) Text_KeyPrompt->SetText(InKeyText);

	if (PB_Cooldown) PB_Cooldown->SetVisibility(ESlateVisibility::Hidden);
	if (Text_CooldownCount) Text_CooldownCount->SetVisibility(ESlateVisibility::Hidden);

	bIsOnCooldown = false;
}

void USkillSlotBase::StartCooldown(float InDuration)
{
	// 이미 쿨타임 중이라면 즉시 종료
	if (bIsOnCooldown)
	{
		
		return;
	}
	
	if (InDuration <= 0.0f) return;

	CooldownTotalDuration = InDuration;
	CooldownEndTime = GetWorld()->GetTimeSeconds() + InDuration; // 목표 시간 설정 -> 쿨타임
	bIsOnCooldown = true;

	if (PB_Cooldown) PB_Cooldown->SetVisibility(ESlateVisibility::Visible);
	if (Text_CooldownCount) Text_CooldownCount->SetVisibility(ESlateVisibility::Visible);
}


