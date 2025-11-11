// Copyright Epic Games, Inc. All Rights Reserved.

#include "SFCameraMode.h"

#include "Components/CapsuleComponent.h"
#include "Engine/Canvas.h"
#include "GameFramework/Character.h"
#include "SFCameraComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFCameraMode)


//////////////////////////////////////////////////////////////////////////
// FSFCameraModeView (카메라 뷰 데이터 구조체)
//////////////////////////////////////////////////////////////////////////
FSFCameraModeView::FSFCameraModeView()
	: Location(ForceInit)
	, Rotation(ForceInit)
	, ControlRotation(ForceInit)
	, FieldOfView(SF_CAMERA_DEFAULT_FOV)
{
}

// 이 뷰(A)와 다른 뷰(B)를 B의 가중치(OtherWeight)만큼 섞음 (블렌딩)
void FSFCameraModeView::Blend(const FSFCameraModeView& Other, float OtherWeight)
{
	if (OtherWeight <= 0.0f)
	{
		// B의 가중치가 0이면 A 유지
		return;
	}
	else if (OtherWeight >= 1.0f)
	{
		// B의 가중치가 1이면 B로 덮어쓰기
		*this = Other;
		return;
	}

	// 위치 (Location) 선형 보간 (Lerp)
	Location = FMath::Lerp(Location, Other.Location, OtherWeight);

	// 회전 (Rotation) 보간
	const FRotator DeltaRotation = (Other.Rotation - Rotation).GetNormalized();
	Rotation = Rotation + (OtherWeight * DeltaRotation);

	// 조작 회전 (ControlRotation) 보간
	const FRotator DeltaControlRotation = (Other.ControlRotation - ControlRotation).GetNormalized();
	ControlRotation = ControlRotation + (OtherWeight * DeltaControlRotation);

	// 시야각 (FieldOfView) 선형 보간
	FieldOfView = FMath::Lerp(FieldOfView, Other.FieldOfView, OtherWeight);
}


//////////////////////////////////////////////////////////////////////////
// USFCameraMode
//////////////////////////////////////////////////////////////////////////
USFCameraMode::USFCameraMode()
{
	// 기본 시야각 및 상하 각도 제한 설정
	FieldOfView = SF_CAMERA_DEFAULT_FOV;
	ViewPitchMin = SF_CAMERA_DEFAULT_PITCH_MIN;
	ViewPitchMax = SF_CAMERA_DEFAULT_PITCH_MAX;

	// 기본 블렌드(전환) 설정: 0.5초, EaseOut 방식
	BlendTime = 0.5f;
	BlendFunction = ESFCameraModeBlendFunction::EaseOut;
	BlendExponent = 4.0f;
	BlendAlpha = 1.0f;
	BlendWeight = 1.0f;
}

// 이 카메라 모드를 소유한 SFCameraComponent 반환
USFCameraComponent* USFCameraMode::GetSFCameraComponent() const
{
	return CastChecked<USFCameraComponent>(GetOuter());
}

// 월드(레벨) 정보 반환
UWorld* USFCameraMode::GetWorld() const
{
	return HasAnyFlags(RF_ClassDefaultObject) ? nullptr : GetOuter()->GetWorld();
}

// 카메라가 바라볼 대상 액터(캐릭터 등) 반환
AActor* USFCameraMode::GetTargetActor() const
{
	const USFCameraComponent* SFCameraComponent = GetSFCameraComponent();
	return SFCameraComponent->GetTargetActor();
}

// 카메라의 중심축(피벗) 위치 계산
FVector USFCameraMode::GetPivotLocation() const
{
	const AActor* TargetActor = GetTargetActor();
	check(TargetActor);

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		// 대상이 캐릭터일 경우, 앉기 상태 등을 고려한 높이 보정
		if (const ACharacter* TargetCharacter = Cast<ACharacter>(TargetPawn))
		{
			// 1순위: 지정된 'CameraSocketName' 소켓 위치 사용
			if (TargetCharacter->GetMesh()->DoesSocketExist(CameraSocketName))
			{
				return TargetCharacter->GetMesh()->GetSocketLocation(CameraSocketName);
			}
			else
			{
				// 2순위: 소켓이 없으면, 캡슐의 높이 변화(앉기 등)를 계산하여 눈높이 보정
				const ACharacter* TargetCharacterCDO = TargetCharacter->GetClass()->GetDefaultObject<ACharacter>();
				check(TargetCharacterCDO);

				const UCapsuleComponent* CapsuleComp = TargetCharacter->GetCapsuleComponent();
				check(CapsuleComp);

				const UCapsuleComponent* CapsuleCompCDO = TargetCharacterCDO->GetCapsuleComponent();
				check(CapsuleCompCDO);

				const float DefaultHalfHeight = CapsuleCompCDO->GetUnscaledCapsuleHalfHeight();
				const float ActualHalfHeight = CapsuleComp->GetUnscaledCapsuleHalfHeight();
				const float HeightAdjustment = (DefaultHalfHeight - ActualHalfHeight) + TargetCharacterCDO->BaseEyeHeight;

				return TargetCharacter->GetActorLocation() + (FVector::UpVector * HeightAdjustment);
			}
		}
		
		// 캐릭터가 아니면 폰(Pawn)의 기본 뷰 위치 사용
		return TargetPawn->GetPawnViewLocation();
	}
	
	// 폰이 아니면 액터의 위치 사용
	return TargetActor->GetActorLocation();
}

// 카메라의 중심축(피벗) 회전값 계산 (플레이어의 조작 방향)
FRotator USFCameraMode::GetPivotRotation() const
{
	const AActor* TargetActor = GetTargetActor();
	check(TargetActor);

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		// 폰(Pawn)의 뷰 회전(컨트롤러의 회전) 사용
		return TargetPawn->GetViewRotation();
	}
	
	// 폰이 아니면 액터의 회전 사용
	return TargetActor->GetActorRotation();
}

// 이 카메라 모드의 매 프레임 업데이트
void USFCameraMode::UpdateCameraMode(float DeltaTime)
{
	UpdateView(DeltaTime);      // 뷰(시점) 계산
	UpdateBlending(DeltaTime);  // 블렌드(전환) 상태 계산
}

// 뷰(시점) 계산 (기본 구현)
void USFCameraMode::UpdateView(float DeltaTime)
{
	// 피벗 위치와 회전값 가져오기
	FVector PivotLocation = GetPivotLocation();
	FRotator PivotRotation = GetPivotRotation();

	// Pitch(상하 각도)를 최소/최대값 사이로 제한
	PivotRotation.Pitch = FMath::ClampAngle(PivotRotation.Pitch, ViewPitchMin, ViewPitchMax);

	// 계산된 값을 최종 뷰(View)에 저장
	View.Location = PivotLocation;
	View.Rotation = PivotRotation;
	View.ControlRotation = View.Rotation;
	View.FieldOfView = FieldOfView;
}

// 블렌드 가중치(0.0~1.0)를 강제로 설정
void USFCameraMode::SetBlendWeight(float Weight)
{
	BlendWeight = FMath::Clamp(Weight, 0.0f, 1.0f);

	// 블렌드 가중치(결과값)를 직접 설정했으므로,
	// 블렌드 알파(진행도)를 블렌드 함수(EaseIn/Out 등)로 역산해야 함.
	const float InvExponent = (BlendExponent > 0.0f) ? (1.0f / BlendExponent) : 1.0f;

	switch (BlendFunction)
	{
	case ESFCameraModeBlendFunction::Linear:
		BlendAlpha = BlendWeight;
		break;
	case ESFCameraModeBlendFunction::EaseIn:
		BlendAlpha = FMath::InterpEaseIn(0.0f, 1.0f, BlendWeight, InvExponent);
		break;
	case ESFCameraModeBlendFunction::EaseOut:
		BlendAlpha = FMath::InterpEaseOut(0.0f, 1.0f, BlendWeight, InvExponent);
		break;
	case ESFCameraModeBlendFunction::EaseInOut:
		BlendAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, BlendWeight, InvExponent);
		break;
	default:
		checkf(false, TEXT("SetBlendWeight: 잘못된 블렌드 함수 [%d]\n"), (uint8)BlendFunction);
		break;
	}
}

// 블렌드(전환) 상태 업데이트
void USFCameraMode::UpdateBlending(float DeltaTime)
{
	if (BlendTime > 0.0f)
	{
		// 설정된 BlendTime에 따라 알파(진행도) 증가
		BlendAlpha += (DeltaTime / BlendTime);
		BlendAlpha = FMath::Min(BlendAlpha, 1.0f);
	}
	else
	{
		// BlendTime이 0이면 즉시 1.0
		BlendAlpha = 1.0f;
	}

	// 블렌드 함수(EaseIn/Out 등)의 곡률(Exponent)
	const float Exponent = (BlendExponent > 0.0f) ? BlendExponent : 1.0f;

	// 계산된 알파(진행도)를 블렌드 함수에 적용하여 최종 가중치(Weight) 계산
	switch (BlendFunction)
	{
	case ESFCameraModeBlendFunction::Linear:
		BlendWeight = BlendAlpha;
		break;
	case ESFCameraModeBlendFunction::EaseIn:
		BlendWeight = FMath::InterpEaseIn(0.0f, 1.0f, BlendAlpha, Exponent);
		break;
	case ESFCameraModeBlendFunction::EaseOut:
		BlendWeight = FMath::InterpEaseOut(0.0f, 1.0f, BlendAlpha, Exponent);
		break;
	case ESFCameraModeBlendFunction::EaseInOut:
		BlendWeight = FMath::InterpEaseInOut(0.0f, 1.0f, BlendAlpha, Exponent);
		break;
	default:
		checkf(false, TEXT("UpdateBlending: 잘못된 블렌드 함수 [%d]\n"), (uint8)BlendFunction);
		break;
	}
}

// 디버그 정보 표시
void USFCameraMode::DrawDebug(UCanvas* Canvas) const
{
	check(Canvas);
	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	DisplayDebugManager.SetDrawColor(FColor::White);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("      SFCameraMode: %s (%f)"), *GetName(), BlendWeight));
}


//////////////////////////////////////////////////////////////////////////
// USFCameraModeStack (카메라 모드 관리 스택)
//////////////////////////////////////////////////////////////////////////
USFCameraModeStack::USFCameraModeStack()
{
	bIsActive = true;
}

// 카메라 모드 스택 활성화
void USFCameraModeStack::ActivateStack()
{
	if (!bIsActive)
	{
		bIsActive = true;

		// 스택에 있는 모든 카메라 모드에게 활성화 알림
		for (USFCameraMode* CameraMode : CameraModeStack)
		{
			check(CameraMode);
			CameraMode->OnActivation();
		}
	}
}

// 카메라 모드 스택 비활성화
void USFCameraModeStack::DeactivateStack()
{
	if (bIsActive)
	{
		bIsActive = false;

		// 스택에 있는 모든 카메라 모드에게 비활성화 알림
		for (USFCameraMode* CameraMode : CameraModeStack)
		{
			check(CameraMode);
			CameraMode->OnDeactivation();
		}
	}
}

// 스택의 맨 위에 새 카메라 모드를 추가 (Push)
void USFCameraModeStack::PushCameraMode(TSubclassOf<USFCameraMode> CameraModeClass)
{
	if (!CameraModeClass)
	{
		return;
	}

	// 카메라 모드 인스턴스(객체) 가져오기
	USFCameraMode* CameraMode = GetCameraModeInstance(CameraModeClass);
	check(CameraMode);

	int32 StackSize = CameraModeStack.Num();

	// 이미 스택의 최상단(0번)인지 확인
	if ((StackSize > 0) && (CameraModeStack[0] == CameraMode))
	{
		// 이미 최상단이므로 아무것도 안 함
		return;
	}

	// 이미 스택 내 다른 위치에 있는지 확인 후 제거
	// (예: 조준 모드 -> 줌 모드 -> 다시 조준 모드)
	// 기존 스택에서 얼마나 기여하고 있었는지(가중치) 계산
	int32 ExistingStackIndex = INDEX_NONE;
	float ExistingStackContribution = 1.0f;

	for (int32 StackIndex = 0; StackIndex < StackSize; ++StackIndex)
	{
		if (CameraModeStack[StackIndex] == CameraMode)
		{
			// 발견. 인덱스 저장 후 반복 중지
			ExistingStackIndex = StackIndex;
			ExistingStackContribution *= CameraMode->GetBlendWeight();
			break;
		}
		else
		{
			// 발견 못함. 상위 모드의 (1.0 - 가중치)만큼 누적
			ExistingStackContribution *= (1.0f - CameraModeStack[StackIndex]->GetBlendWeight());
		}
	}

	if (ExistingStackIndex != INDEX_NONE)
	{
		// 기존 위치에서 제거
		CameraModeStack.RemoveAt(ExistingStackIndex);
		StackSize--;
	}
	else
	{
		// 스택에 없었음. 기여도 0
		ExistingStackContribution = 0.0f;
	}

	// 시작할 초기 가중치 결정
	// (BlendTime이 있고 스택에 다른 모드가 있으면) 부드러운 전환(블렌드) 시작
	const bool bShouldBlend = ((CameraMode->GetBlendTime() > 0.0f) && (StackSize > 0));
	const float BlendWeight = (bShouldBlend ? ExistingStackContribution : 1.0f);

	CameraMode->SetBlendWeight(BlendWeight);

	// 새 항목을 스택 최상단(0번 인덱스)에 추가
	CameraModeStack.Insert(CameraMode, 0);

	// 스택의 맨 아래(Last)는 항상 100% 가중치를 갖도록 보장
	CameraModeStack.Last()->SetBlendWeight(1.0f);

	// 스택에 '새로' 추가된 경우(기존에 없던 경우) 활성화 알림
	if (ExistingStackIndex == INDEX_NONE)
	{
		CameraMode->OnActivation();
	}
}

// 스택을 평가(계산)하여 최종 카메라 뷰 결과물 반환
bool USFCameraModeStack::EvaluateStack(float DeltaTime, FSFCameraModeView& OutCameraModeView)
{
	if (!bIsActive)
	{
		return false;
	}

	UpdateStack(DeltaTime);      // 스택 내 각 모드 업데이트
	BlendStack(OutCameraModeView); // 스택 블렌딩

	return true;
}

// 카메라 모드 클래스(설계도)로부터 실제 인스턴스(객체)를 가져오거나 생성
USFCameraMode* USFCameraModeStack::GetCameraModeInstance(TSubclassOf<USFCameraMode> CameraModeClass)
{
	check(CameraModeClass);

	// 1. 이미 생성된 인스턴스가 있는지 확인 (재사용)
	for (USFCameraMode* CameraMode : CameraModeInstances)
	{
		if ((CameraMode != nullptr) && (CameraMode->GetClass() == CameraModeClass))
		{
			return CameraMode;
		}
	}

	// 2. 발견되지 않음. 새로 생성 필요.
	USFCameraMode* NewCameraMode = NewObject<USFCameraMode>(GetOuter(), CameraModeClass, NAME_None, RF_NoFlags);
	check(NewCameraMode);

	CameraModeInstances.Add(NewCameraMode); // 재사용을 위해 목록에 추가

	return NewCameraMode;
}

// 스택 내 각 모드 업데이트 및 정리
void USFCameraModeStack::UpdateStack(float DeltaTime)
{
	const int32 StackSize = CameraModeStack.Num();
	if (StackSize <= 0)
	{
		return;
	}

	int32 RemoveCount = 0;
	int32 RemoveIndex = INDEX_NONE;

	// 스택 위에서부터(0번 인덱스) 순회
	for (int32 StackIndex = 0; StackIndex < StackSize; ++StackIndex)
	{
		USFCameraMode* CameraMode = CameraModeStack[StackIndex];
		check(CameraMode);

		// 각 모드의 업데이트 실행 (뷰 계산, 블렌드 알파 증가 등)
		CameraMode->UpdateCameraMode(DeltaTime);

		if (CameraMode->GetBlendWeight() >= 1.0f)
		{
			// 이 모드의 가중치가 100%가 됨.
			// 즉, 이 모드보다 '아래'(더 높은 인덱스)에 있는 모든 모드는
			// 이제 영향력이 0%이므로 스택에서 제거해도 됨.
			RemoveIndex = (StackIndex + 1);
			RemoveCount = (StackSize - RemoveIndex);
			break;
		}
	}

	if (RemoveCount > 0)
	{
		// 스택에서 제거됨을 카메라 모드들에 알림
		for (int32 StackIndex = RemoveIndex; StackIndex < StackSize; ++StackIndex)
		{
			USFCameraMode* CameraMode = CameraModeStack[StackIndex];
			check(CameraMode);

			CameraMode->OnDeactivation();
		}

		// 실제 스택에서 제거
		CameraModeStack.RemoveAt(RemoveIndex, RemoveCount);
	}
}

// 스택에 쌓인 모드들을 블렌딩하여 최종 뷰 생성
void USFCameraModeStack::BlendStack(FSFCameraModeView& OutCameraModeView) const
{
	const int32 StackSize = CameraModeStack.Num();
	if (StackSize <= 0)
	{
		return;
	}

	// 스택의 맨 아래(가장 마지막 인덱스, 가중치 100%)에서 시작
	const USFCameraMode* CameraMode = CameraModeStack[StackSize - 1];
	check(CameraMode);

	OutCameraModeView = CameraMode->GetCameraModeView();

	// 스택을 위로 올라오면서(인덱스가 작아지면서) 덮어씌우며 블렌딩
	for (int32 StackIndex = (StackSize - 2); StackIndex >= 0; --StackIndex)
	{
		CameraMode = CameraModeStack[StackIndex];
		check(CameraMode);

		// (아래쪽 뷰)와 (현재 뷰)를 (현재 뷰의 가중치)만큼 섞음
		OutCameraModeView.Blend(CameraMode->GetCameraModeView(), CameraMode->GetBlendWeight());
	}
}

// 디버그 정보 표시 (스택 순서대로)
void USFCameraModeStack::DrawDebug(UCanvas* Canvas) const
{
	check(Canvas);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	DisplayDebugManager.SetDrawColor(FColor::Green);
	DisplayDebugManager.DrawString(FString(TEXT("   --- 카메라 모드 스택 (시작) ---")));

	for (const USFCameraMode* CameraMode : CameraModeStack)
	{
		check(CameraMode);
		CameraMode->DrawDebug(Canvas); // 각 모드의 디버그 정보 표시
	}

	DisplayDebugManager.SetDrawColor(FColor::Green);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("   --- 카메라 모드 스택 (끝) ---")));
}

// 스택 최상단(가장 마지막에 Push된) 모드의 정보 반환
void USFCameraModeStack::GetBlendInfo(float& OutWeightOfTopLayer, FGameplayTag& OutTagOfTopLayer) const
{
	if (CameraModeStack.Num() == 0)
	{
		OutWeightOfTopLayer = 1.0f;
		OutTagOfTopLayer = FGameplayTag();
		return;
	}
	else
	{
		// 주의: 스택의 최상단은 0번 인덱스이지만,
		// 라이라 원본 코드는 Last() (맨 아래)를 반환함. 여기서는 코드를 그대로 따름.
		// (만약 최상단(0번)을 원한다면 CameraModeStack[0] 사용)
		USFCameraMode* TopEntry = CameraModeStack.Last(); 
		check(TopEntry);
		OutWeightOfTopLayer = TopEntry->GetBlendWeight();
		OutTagOfTopLayer = TopEntry->GetCameraTypeTag();
	}
}

