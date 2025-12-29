#include "UI/InGame/SFIndicatorWidgetBase.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Widget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"

void USFIndicatorWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	// 자주 쓰는 Canvas Slot을 미리 캐싱해둠 (매번 Cast 안 하도록)
	if (IndicatorRoot)
	{
		RootCanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(IndicatorRoot);
	}
	
}

void USFIndicatorWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 1. 필수 컴포넌트 유효성 검사
	if (!IndicatorRoot || !ArrowImage || !NameText || !RootCanvasSlot)
	{
		return;
	}

	// 2. 핵심 로직 함수 호출
	UpdateIndicatorTransform();
}

void USFIndicatorWidgetBase::SetTargetActor(AActor* InTargetActor)
{
	TargetActorPtr = InTargetActor;

	// 타겟이 들어오면 이름표 미리 세팅
	UpdatePlayerName();
}

void USFIndicatorWidgetBase::UpdatePlayerName()
{
	AActor* Target = TargetActorPtr.Get();
	if (!Target || !NameText) return;

	if (APawn* TargetPawn = Cast<APawn>(Target))
	{
		// PlayerState가 생성되었는지 확인
		if (APlayerState* PS = TargetPawn->GetPlayerState())
		{
			FString PlayerName = PS->GetPlayerName();
			// 유효한 이름이 들어왔을 때만 텍스트 설정
			if (!PlayerName.IsEmpty())
			{
				NameText->SetText(FText::FromString(PlayerName));
			}
		}
		else
		{
			// PlayerState가 아직 없다면 -> 공백란
			NameText->SetText(FText::GetEmpty());
		}
	}
}

void USFIndicatorWidgetBase::UpdateIndicatorTransform()
{
	AActor* Target = TargetActorPtr.Get();
    APlayerController* PC = GetOwningPlayer();

    // 1. 타겟/PC 유효성 검사
    if (!Target || !PC)
    {
       SetRenderOpacity(0.0f);
       return;
    }
    SetRenderOpacity(1.0f);

    // 2. 위치 계산 (머리 위 +120.0f 보정)
    FVector TargetLocation = Target->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f);
    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

	// 카메라와 타겟 사이의 거리 계산 (단위: cm)
	float CamDistance = FVector::Dist(CameraLocation, TargetLocation);

	// 거리 범위 설정 (예: 5m ~ 50m)
	const float MinDistance = 500.0f;  // 가까울 때 (5m)
	const float MaxDistance = 5000.0f; // 멀 때 (50m)

	// 스케일 범위 설정 (1.0배 ~ 0.6배)
	const float MinScale = 0.6f;
	const float MaxScale = 1.0f;

	// 거리를 스케일 값으로 매핑 (가까우면 MaxScale, 멀면 MinScale)
	// GetMappedRangeValueClamped: 입력값이 범위 밖이어도 알아서 최소/최대로 설정
	float DistanceScale = FMath::GetMappedRangeValueClamped(
		FVector2D(MinDistance, MaxDistance),
		FVector2D(MaxScale, MinScale),
		CamDistance
		);

	// 위젯 전체 루트에 스케일 적용
	IndicatorRoot->SetRenderScale(FVector2D(DistanceScale, DistanceScale));
	
    // 3. 월드 -> 스크린 변환
    FVector2D ScreenPosition;
    bool bIsOnScreen = UGameplayStatics::ProjectWorldToScreen(PC, TargetLocation, ScreenPosition);

    // 4. 뷰포트 크기 및 DPI 스케일 적용
    int32 SizeX, SizeY;
    PC->GetViewportSize(SizeX, SizeY);
    FVector2D ViewportSize(SizeX, SizeY);

    float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
    if (Scale > 0.0f)
    {
       ScreenPosition /= Scale;
       ViewportSize /= Scale;
    }
    FVector2D ScreenCenter = ViewportSize / 2.0f;

    // 5. 화면 밖 / 카메라 뒤 판정
    FVector DirectionToTarget = (TargetLocation - CameraLocation).GetSafeNormal();
    FVector LocalDirection = CameraRotation.UnrotateVector(DirectionToTarget);

    bool bIsBehind = LocalDirection.X < 0.0f;
    bool bIsWayOut = (ScreenPosition.X < -ScreenEdgeMargin || ScreenPosition.X > ViewportSize.X + ScreenEdgeMargin ||
                      ScreenPosition.Y < -ScreenEdgeMargin || ScreenPosition.Y > ViewportSize.Y + ScreenEdgeMargin);

    bool bIsOffScreen = !bIsOnScreen || bIsBehind || bIsWayOut;

    // 6. 최종 좌표 및 회전 계산
    float RotationAngle = 0.0f;

    if (bIsOffScreen)
    {
       // === [화면 밖] ===
       FVector2D Direction2D(LocalDirection.Y, LocalDirection.Z * -1.0f);
       if (Direction2D.IsNearlyZero()) Direction2D = FVector2D(1, 0);
       Direction2D.Normalize();

       float Radians = FMath::Atan2(Direction2D.Y, Direction2D.X);
       RotationAngle = FMath::RadiansToDegrees(Radians);

       float AngleCos = FMath::Cos(Radians);
       float AngleSin = FMath::Sin(Radians);

    	FVector2D IndiWidgetSize = IndicatorRoot->GetDesiredSize();

    	// 만약 첫 프레임이라 크기가 0이면 기본값(150, 30)으로 방어
    	if (IndiWidgetSize.IsZero()) IndiWidgetSize = FVector2D(150.0f, 30.0f);

    	// 화면 절반 크기에서 "마진"과 "위젯 절반 크기"를 각각 뺍니다.
    	// 가로 벽 거리 = (화면가로/2) - (위젯가로/2) - 마진
    	float BoundX = (ViewportSize.X / 2.0f) - (IndiWidgetSize.X / 2.0f) - ScreenEdgeMargin;
    	// 세로 벽 거리 = (화면세로/2) - (위젯세로/2) - 마진
    	float BoundY = (ViewportSize.Y / 2.0f) - (IndiWidgetSize.Y / 2.0f) - ScreenEdgeMargin;

    	// 혹시 마진이 너무 커서 음수가 되면 0으로 고정 (안전장치)
    	BoundX = FMath::Max(BoundX, 0.0f);
    	BoundY = FMath::Max(BoundY, 0.0f);

    	// 기울기(Slope)를 이용해 부딪히는 지점 계산 (y = mx)
    	// Cos가 0에 가까우면(수직) 무한대가 되므로 예외처리
    	float DistToVerticalEdge = (AngleCos != 0.0f) ? FMath::Abs(BoundX / AngleCos) : 999999.0f;
    	float DistToHorizontalEdge = (AngleSin != 0.0f) ? FMath::Abs(BoundY / AngleSin) : 999999.0f;

    	// 둘 중 더 빨리 벽에 닿는 거리를 선택
    	float FinalDistance = FMath::Min(DistToVerticalEdge, DistToHorizontalEdge);

    	// 최종 좌표 확정
    	ScreenPosition = ScreenCenter + (FVector2D(AngleCos, AngleSin) * FinalDistance);

    	// 비주얼 업데이트 (화면 밖)
    	ArrowImage->SetVisibility(ESlateVisibility::Visible);
    	ArrowImage->SetRenderTransformAngle(RotationAngle + 90.0f);
    	NameText->SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
       // === [화면 안] ===
       ArrowImage->SetVisibility(ESlateVisibility::Hidden);
       NameText->SetVisibility(ESlateVisibility::Visible);

       // 이름 업데이트 (함수 재활용)
       if (NameText->GetText().IsEmpty())
       {
           UpdatePlayerName();
       }

       ArrowImage->SetRenderTransformAngle(0.0f);
    }

	// 이름 업데이트 시도 (아직 안 떴을 경우 대비)
	if (NameText->GetText().IsEmpty())
	{
		UpdatePlayerName();
	}

    // 7. 이동
    RootCanvasSlot->SetPosition(ScreenPosition);
}