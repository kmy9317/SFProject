#include "UI/Common/CommonBarBase.h"

#include <Programs/UnrealBuildAccelerator/Core/Public/UbaBase.h>

#include "ShaderPrintParameters.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
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

	// 이 함수가 처음 호출된 경우 (즉, 스폰 직후)
	if (!bHasInitialized)
	{
		PB_Current->SetPercent(TargetPercent);
		PB_Delayed->SetPercent(TargetPercent);
		
		bHasInitialized = true;
		
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

void UCommonBarBase::SetBarColor(FLinearColor NewColor)
{
	BarFillColor = NewColor;

	if (PB_Current)
	{
		PB_Current->SetFillColorAndOpacity(BarFillColor);
	}
}

void UCommonBarBase::UpdateBarWidth(float MaxStat)
{
	// SizeBox가 없을 경우 (몬스터용 CommonBar) 즉시 리턴
	if (!DynamicSizeBox)
	{
		return;
	}

	// 비율 설정이 잘못되어 있으면(0 이하) 계산 중단
	if (WidthPerStat <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("WidthPerStat is zero or negative!"));
		return;
	}

	float NewWidth = MaxStat * WidthPerStat;

	// 계산된 길이가 지정한 최소길이(Min)보다 작으면 최소길이로, 지정한 최대길이(Max)보다 크면 최대길이로 고정
	NewWidth = FMath::Clamp(NewWidth, MinBarWidth, MaxBarWidth);

	DynamicSizeBox->SetWidthOverride(NewWidth);
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
