#include "SFLockOnComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "SF/Character/SFCharacterBase.h"
#include "SF/Character/SFCharacterGameplayTags.h" 
#include "GenericTeamAgentInterface.h" // 팀 관계 확인용

USFLockOnComponent::USFLockOnComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// 기본 태그 설정
	TargetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Character.Type.Enemy")));
}

void USFLockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CurrentTarget) return;

	// 1. 타겟 유효성 검사
	UpdateLogic_TargetValidation(DeltaTime);
	
	if (!CurrentTarget) return;

	// 2. 타겟 스위칭
	HandleTargetSwitching(DeltaTime);

	// 3. 카메라 회전
	UpdateLogic_CameraRotation(DeltaTime);

	// 4. 캐릭터 회전
	UpdateLogic_CharacterRotation(DeltaTime);

	// 5. 위젯 위치
	UpdateLogic_WidgetPosition(DeltaTime);
}

// =========================================================
//  Logic Implementations
// =========================================================

void USFLockOnComponent::UpdateLogic_TargetValidation(float DeltaTime)
{
	bool bShouldBreak = false;

	// A. 기본 유효성 및 거리 검사
	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), CurrentTarget->GetActorLocation());
	
	// IsTargetValidBasic -> IsTargetValid (Interface 사용) 변경
	if (!IsTargetValid(CurrentTarget) || Distance > LockOnBreakDistance)
	{
		bShouldBreak = true;
	}
	else
	{
		// B. 시야 가림 유예 (Grace Period)
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(GetOwner());
		QueryParams.AddIgnoredActor(CurrentTarget);

		FVector Start = GetOwner()->GetActorLocation() + FVector(0, 0, 50);
		
		// 소켓 정보가 있으면 소켓 위치 사용, 없으면 Actor Location
		FVector End = CurrentTarget->GetActorLocation();
		if (CurrentTargetSocketName != NAME_None)
		{
			if (USceneComponent* Mesh = CurrentTarget->FindComponentByClass<USceneComponent>())
			{
				if (Mesh->DoesSocketExist(CurrentTargetSocketName))
				{
					End = Mesh->GetSocketLocation(CurrentTargetSocketName);
				}
			}
		}
		else
		{
			End += FVector(0, 0, 50);
		}

		bool bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult, Start, End, ECC_Visibility, QueryParams
		);

		if (bHit)
		{
			TimeSinceTargetHidden += DeltaTime;
			if (TimeSinceTargetHidden > LostTargetMemoryTime)
			{
				bShouldBreak = true;
			}
		}
		else
		{
			TimeSinceTargetHidden = 0.0f;
		}
	}

	if (bShouldBreak)
	{
		EndLockOn();
	}
}

void USFLockOnComponent::HandleTargetSwitching(float DeltaTime)
{
	if (CurrentSwitchCooldown > 0.0f)
	{
		CurrentSwitchCooldown -= DeltaTime;
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;
	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC) return;

	// 입력 감지
	float InputX = 0.0f;
	float InputY = 0.0f;
	PC->GetInputAnalogStickState(EControllerAnalogStick::CAS_RightStick, InputX, InputY);

	if (FMath::IsNearlyZero(InputX) && FMath::IsNearlyZero(InputY))
	{
		float MouseX = 0.0f, MouseY = 0.0f;
		PC->GetInputMouseDelta(MouseX, MouseY);
		
		float MouseSensitivity = 0.3f; 
		InputX = MouseX * MouseSensitivity;
		InputY = MouseY * MouseSensitivity;
	}

	FVector2D CurrentInput(InputX, InputY);
	if (CurrentInput.Size() < SwitchInputThreshold) return;

	// 스위칭 로직 (기존과 동일하되 IsTargetValid 사용)
	FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();
	FVector CamRight = CamRot.RotateVector(FVector::RightVector);
	FVector CamUp = CamRot.RotateVector(FVector::UpVector);
	
	FVector SearchDirection = (CamRight * CurrentInput.X + CamUp * CurrentInput.Y).GetSafeNormal();
	
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		this, OwnerPawn->GetActorLocation(), LockOnDistance, ObjectTypes,
		AActor::StaticClass(), { OwnerPawn, CurrentTarget }, OverlappedActors
	);

	AActor* BestNewTarget = nullptr;
	float ClosestDistSq = FLT_MAX; 

	for (AActor* Candidate : OverlappedActors)
	{
		if (!IsTargetValid(Candidate)) continue;
		if (!IsHostile(Candidate)) continue;

		FVector FromTargetToCand = Candidate->GetActorLocation() - CurrentTarget->GetActorLocation();
		float DistSq = FromTargetToCand.SizeSquared(); 
		FVector DirToCand = FromTargetToCand.GetSafeNormal();

		float InputDot = FVector::DotProduct(SearchDirection, DirToCand);

		if (InputDot > SwitchAngularLimit) 
		{
			if (DistSq < ClosestDistSq)
			{
				ClosestDistSq = DistSq;
				BestNewTarget = Candidate;
			}
		}
	}

	if (BestNewTarget)
	{
		CurrentTarget = BestNewTarget;
		CurrentSwitchCooldown = SwitchCooldown;
		TimeSinceTargetHidden = 0.0f;
		
		// 새 타겟의 소켓 정보 갱신
		if (const ISFLockOnInterface* LockOnInterface = Cast<const ISFLockOnInterface>(CurrentTarget))
		{
			TArray<FName> Sockets = LockOnInterface->GetLockOnSockets();
			CurrentTargetSocketName = (Sockets.Num() > 0) ? Sockets[0] : LockOnSocketName;
		}
		else
		{
			CurrentTargetSocketName = LockOnSocketName;
		}

		DestroyLockOnWidget();
		CreateLockOnWidget();

		bIsSwitchingTarget = true;
	}
}

void USFLockOnComponent::UpdateLogic_CameraRotation(float DeltaTime)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;
	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC) return;

	// 1. 타겟 목표 위치 계산 (CurrentTargetSocketName 우선)
	FVector TargetLoc = CurrentTarget->GetActorLocation();
	if (USceneComponent* TargetMesh = CurrentTarget->FindComponentByClass<USceneComponent>())
	{
		FName SocketToUse = (CurrentTargetSocketName != NAME_None) ? CurrentTargetSocketName : LockOnSocketName;
		
		if (TargetMesh->DoesSocketExist(SocketToUse))
		{
			TargetLoc = TargetMesh->GetSocketLocation(SocketToUse);
		}
		else
		{
			TargetLoc.Z += 50.0f; 
		}
	}

	// 2. 목표 회전값 계산
	FVector CameraLoc = PC->PlayerCameraManager->GetCameraLocation();
	FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(CameraLoc, TargetLoc);
	LookAtRot.Pitch = FMath::Clamp(LookAtRot.Pitch, -45.0f, 45.0f);

	// 3. 보간 속도
	float InterpSpeed = 30.0f; 
	if (bIsSwitchingTarget)
	{
		InterpSpeed = TargetSwitchInterpSpeed; 
		FRotator Delta = (LookAtRot - LastLockOnRotation).GetNormalized();
		if (FMath::Abs(Delta.Yaw) < 2.0f && FMath::Abs(Delta.Pitch) < 2.0f)
		{
			bIsSwitchingTarget = false;
		}
	}

	// 4. 회전 적용
	FRotator SmoothRot = FMath::RInterpTo(LastLockOnRotation, LookAtRot, DeltaTime, InterpSpeed);
	PC->SetControlRotation(SmoothRot);
	LastLockOnRotation = SmoothRot; 
}

void USFLockOnComponent::UpdateLogic_CharacterRotation(float DeltaTime)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;
	ACharacter* Character = Cast<ACharacter>(OwnerPawn);
	if (!Character) return;

	bool bIsSprinting = false;
	if (const IGameplayTagAssetInterface* TagInterface = Cast<const IGameplayTagAssetInterface>(Character))
	{
		if (SprintTag.IsValid() && TagInterface->HasMatchingGameplayTag(SprintTag))
		{
			bIsSprinting = true;
		}
	}

	if (bIsSprinting)
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = true;
	}
	else
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		FRotator TargetRot = FRotator(0.0f, LastLockOnRotation.Yaw, 0.0f);
		FRotator SmoothRot = FMath::RInterpTo(Character->GetActorRotation(), TargetRot, DeltaTime, 15.0f);
		Character->SetActorRotation(SmoothRot);
	}
}

void USFLockOnComponent::UpdateLogic_WidgetPosition(float DeltaTime)
{
	// Widget BP에서 처리
}

// =========================================================
//  Main Functions
// =========================================================

bool USFLockOnComponent::TryLockOn()
{
	// 1. 쿨타임 체크 (연타 방지)
	double CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastLockOnToggleTime < 0.2)
	{
		return true; // 쿨타임 중엔 처리된 것으로 간주 (리셋 방지)
	}

	// 2. 이미 락온 중 -> 해제 (Toggle Off)
	if (CurrentTarget)
	{
		EndLockOn();
		LastLockOnToggleTime = CurrentTime;
		return true; // "해제 성공"도 true 반환
	}

	// 3. 새로운 타겟 탐색
	AActor* NewTarget = FindBestTarget();
	if (NewTarget)
	{
		CurrentTarget = NewTarget;
		TimeSinceTargetHidden = 0.0f;
		
		// 타겟 소켓 설정
		if (const ISFLockOnInterface* LockOnInterface = Cast<const ISFLockOnInterface>(CurrentTarget))
		{
			TArray<FName> Sockets = LockOnInterface->GetLockOnSockets();
			CurrentTargetSocketName = (Sockets.Num() > 0) ? Sockets[0] : LockOnSocketName;
		}
		else
		{
			CurrentTargetSocketName = LockOnSocketName;
		}

		if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerPawn))
			{
				ASC->AddLooseGameplayTag(SFGameplayTags::Character_State_LockedOn);
			}

			if (ACharacter* Character = Cast<ACharacter>(OwnerPawn))
			{
				// 마우스 입력이 캐릭터 회전에 영향 주지 않도록 연결 끊기
				Character->bUseControllerRotationYaw = false;
				Character->GetCharacterMovement()->bOrientRotationToMovement = false;
				Character->GetCharacterMovement()->MaxWalkSpeed *= 0.8f; 
			}

			// 초기화: 현재 뷰를 시작점으로 설정
			if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
			{
				LastLockOnRotation = PC->GetControlRotation();
			}
			bIsSwitchingTarget = false;
		}

		CreateLockOnWidget();
		LastLockOnToggleTime = CurrentTime;
		return true; // 락온 성공
	}

	// 4. 타겟 없음 -> 카메라 리셋 필요 (false 반환)
	return false;
}

void USFLockOnComponent::EndLockOn()
{
	DestroyLockOnWidget();
	CurrentTarget = nullptr;
	CurrentTargetSocketName = NAME_None;
	TimeSinceTargetHidden = 0.0f;

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		// GAS 태그 제거
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerPawn))
		{
			ASC->RemoveLooseGameplayTag(SFGameplayTags::Character_State_LockedOn);
		}

		// 캐릭터 상태 복구
		if (ACharacter* Character = Cast<ACharacter>(OwnerPawn))
		{
			Character->bUseControllerRotationYaw = false;
			Character->GetCharacterMovement()->bOrientRotationToMovement = true;
			Character->GetCharacterMovement()->MaxWalkSpeed /= 0.8f;
		}
	}
}

// =========================================================
//  Helper Functions (Refactored)
// =========================================================

bool USFLockOnComponent::IsTargetValid(AActor* TargetActor) const
{
	if (!TargetActor) return false;

	// 1. Interface Check (가장 우선)
	if (const ISFLockOnInterface* LockOnTarget = Cast<const ISFLockOnInterface>(TargetActor))
	{
		if (!LockOnTarget->CanBeLockedOn()) return false;
		return true; // 인터페이스가 있다면 그 결과만 따름
	}

	// 2. Fallback: SFCharacterBase Check
	if (const ASFCharacterBase* SFChar = Cast<ASFCharacterBase>(TargetActor))
	{
		if (!SFChar->IsAlive()) return false;
		return true; // SF 캐릭터라면 생존 여부만 따름
	}

	// 3. 그 외의 경우 (인터페이스도 없고 SF캐릭터도 아님)
	return false; 
}

bool USFLockOnComponent::IsHostile(AActor* TargetActor) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return false;

	// IGenericTeamAgentInterface 활용
	const IGenericTeamAgentInterface* OwnerTeamAgent = Cast<const IGenericTeamAgentInterface>(OwnerPawn);
	const IGenericTeamAgentInterface* TargetTeamAgent = Cast<const IGenericTeamAgentInterface>(TargetActor);

	if (OwnerTeamAgent && TargetTeamAgent)
	{
		// ETeamAttitude::Hostile 인 경우만 True
		return OwnerTeamAgent->GetTeamAttitudeTowards(*TargetActor) == ETeamAttitude::Hostile;
	}

	return true; // 인터페이스 없으면 기본적으로 타겟 가능으로 간주 (또는 태그 체크)
}

AActor* USFLockOnComponent::FindBestTarget()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return nullptr;
	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC) return nullptr;

	FVector CameraLoc;
	FRotator CameraRot;
	PC->GetPlayerViewPoint(CameraLoc, CameraRot);
	FVector CameraForward = CameraRot.Vector();

	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		this, OwnerPawn->GetActorLocation(), LockOnDistance, ObjectTypes,
		AActor::StaticClass(), { OwnerPawn }, OverlappedActors
	);

	AActor* BestTarget = nullptr;
	float BestScore = -1.0f;

	for (AActor* Candidate : OverlappedActors)
	{
		if (!IsTargetValid(Candidate)) continue;
		if (!IsHostile(Candidate)) continue;

		// 1. 화면 내 존재 여부 확인 (내적)
		FVector DirToTarget = (Candidate->GetActorLocation() - CameraLoc).GetSafeNormal();
		float DotResult = FVector::DotProduct(CameraForward, DirToTarget);

		// 화면 중앙 범위 밖이면 제외
		if (DotResult < ScreenCenterWeight) continue;

		// 2. 점수 계산 (Score Calculation)
		float Score = 0.0f;

		// [A] 거리 점수 (가까울수록 높음)
		float Distance = FVector::Dist(OwnerPawn->GetActorLocation(), Candidate->GetActorLocation());
		float DistScore = FMath::Clamp(1.0f - (Distance / LockOnDistance), 0.0f, 1.0f);
		Score += DistScore * Weight_Distance;

		// [B] 각도 점수 (화면 중앙일수록 높음)
		float AngleScore = (DotResult - ScreenCenterWeight) / (1.0f - ScreenCenterWeight);
		Score += AngleScore * Weight_Angle;

		// [C] 보스 우선 가중치
		// 보스 태그 확인 (직접 TagInterface 체크)
		if (const IGameplayTagAssetInterface* TagInterface = Cast<const IGameplayTagAssetInterface>(Candidate))
		{
			// "Character.Type.Boss" 태그가 있다고 가정
			if (TagInterface->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Character.Type.Boss"))))
			{
				Score += Weight_BossBonus;
			}
		}

		// [D] 시야 가림 검사 (Raycast)
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(OwnerPawn);
		Params.AddIgnoredActor(Candidate);
		
		FVector TestTargetLoc = Candidate->GetActorLocation();
		// 소켓 정보가 있다면 첫 번째 소켓 위치를 기준으로 시야 검사
		if (const ISFLockOnInterface* LockOnInterface = Cast<const ISFLockOnInterface>(Candidate))
		{
			TArray<FName> Sockets = LockOnInterface->GetLockOnSockets();
			if (Sockets.Num() > 0)
			{
				if (USceneComponent* Mesh = Candidate->FindComponentByClass<USceneComponent>())
				{
					TestTargetLoc = Mesh->GetSocketLocation(Sockets[0]);
				}
			}
		}
		else
		{
			TestTargetLoc.Z += 50.0f;
		}

		bool bVisible = !GetWorld()->LineTraceSingleByChannel(
			Hit, CameraLoc, TestTargetLoc, ECC_Visibility, Params
		);

		if (!bVisible)
		{
			Score *= 0.5f; // 가려져 있으면 점수 절반 차감 (제외하지 않고 우선순위만 낮춤)
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

void USFLockOnComponent::CreateLockOnWidget()
{
	if (!LockOnWidgetClass || !GetWorld()) return;

	if (!LockOnWidgetInstance)
	{
		LockOnWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), LockOnWidgetClass);
	}

	if (LockOnWidgetInstance && !LockOnWidgetInstance->IsInViewport())
	{
		LockOnWidgetInstance->AddToViewport(-1);
	}
}

void USFLockOnComponent::DestroyLockOnWidget()
{
	if (LockOnWidgetInstance)
	{
		LockOnWidgetInstance->RemoveFromParent();
		LockOnWidgetInstance = nullptr;
	}
}