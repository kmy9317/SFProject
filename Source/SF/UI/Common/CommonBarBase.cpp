#include "UI/Common/CommonBarBase.h"

#include "ShaderPrintParameters.h"
#include "Components/ProgressBar.h"
#include "Kismet/KismetMathLibrary.h" // NearlyEqual 사용

void UCommonBarBase::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsValid(PB_Current))
	{
		PB_Current->SetFillColorAndOpacity(BarFillColor);
	}
}


void UCommonBarBase::SetPercentVisuals(float InPercent)
{
	TargetPercent = InPercent;

	if (!PB_Current || !PB_Delayed)
	{
		return;
	}

	// 현재 수치 저장
	const float CurrentPercent = PB_Current->GetPercent();

	// 입력값 > 현재값 == 회복
	if (TargetPercent > CurrentPercent)
	{
		bIsRecovering = true;
		PB_Delayed->SetPercent(TargetPercent); // DelayedBar 즉시 채우기
		bIsDecreasing = false;
	}
	// 입력값 < 현재값 == 감소
	if (TargetPercent < CurrentPercent)
	{
		bIsRecovering = false;
		PB_Current->SetPercent(TargetPercent); // CurrentBar 즉시 채우기
		bIsDecreasing = true;
	}
}

void UCommonBarBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!PB_Current || !PB_Delayed)
	{
		return;
	}

	if (bIsRecovering)
	{
		const float CurrentPercent = PB_Current->GetPercent();

		const float NewCurrent = FMath::FInterpTo(CurrentPercent, TargetPercent, InDeltaTime, InterpSpeedToRecover);

		PB_Current->SetPercent(NewCurrent);

		if (FMath::IsNearlyEqual(NewCurrent, TargetPercent))
		{
			bIsRecovering = false;
			PB_Delayed->SetPercent(TargetPercent);
		}
	}
	if (bIsDecreasing)
	{
		const float DelayedPercent = PB_Delayed->GetPercent();

		const float NewDelayed = FMath::FInterpTo(DelayedPercent, TargetPercent, InDeltaTime, InterpSpeedToDecrease);

		PB_Delayed->SetPercent(NewDelayed);

		if (FMath::IsNearlyEqual(NewDelayed, TargetPercent))
		{
			bIsDecreasing = false;
		}
	}
}
