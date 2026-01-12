#include "SFLockOnComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "SF/Character/SFCharacterBase.h"
#include "SF/Character/SFCharacterGameplayTags.h" 
#include "GenericTeamAgentInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "Character/Enemy/SFEnemy.h"
#include "Net/UnrealNetwork.h"

USFLockOnComponent::USFLockOnComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
	
	TargetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Character.Type.Enemy")));
}

void USFLockOnComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USFLockOnComponent, CurrentTarget);
	DOREPLIFETIME(USFLockOnComponent, CurrentTargetSocketName);
}

void USFLockOnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 컴포넌트 파괴 시 이벤트 및 이펙트 정리
	if (CurrentTarget)
	{
		UnregisterTargetEvents(CurrentTarget);
	}
	DestroyLockOnEffect();
	Super::EndPlay(EndPlayReason);
}

void USFLockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;

    // [Server] 타겟 유효성 검사
    if (OwnerPawn->HasAuthority())
    {
        ServerUpdate_TargetValidation(DeltaTime);
        
        // 서버에서 원격 클라이언트 회전 처리
        if (CurrentTarget && IsValid(CurrentTarget) && !OwnerPawn->IsLocallyControlled())
        {
            ServerUpdate_RemoteClientRotation(DeltaTime);
        }
    }

    // [Client] 로컬 플레이어 로직
    if (OwnerPawn->IsLocallyControlled())
    {
        if (CurrentTarget && IsValid(CurrentTarget))
        {
            ClientUpdate_SwitchingInput(DeltaTime);
            ClientUpdate_Rotation(DeltaTime);
            UpdateLockOnEffectLocation();
        }
        else
        {
            // 락온 해제 시 CMC 상태 확인 및 복원
            EnsureDefaultRotationMode();
            
            // 카메라 리셋 로직
            if (bIsResettingCamera)
            {
                if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
                {
                    // 입력 감지 시 리셋 중단
                    float MouseX = 0.0f, MouseY = 0.0f;
                    PC->GetInputMouseDelta(MouseX, MouseY);
                    
                    float StickX = 0.0f, StickY = 0.0f;
                    PC->GetInputAnalogStickState(EControllerAnalogStick::CAS_RightStick, StickX, StickY);

                    if (FMath::Abs(MouseX) > 0.1f || FMath::Abs(MouseY) > 0.1f || 
                        FMath::Abs(StickX) > 0.1f || FMath::Abs(StickY) > 0.1f)
                    {
                        bIsResettingCamera = false;
                        return;
                    }

                    // 리셋 보간
                    FRotator CurrentRot = PC->GetControlRotation();
                    FRotator NewRot = FMath::RInterpTo(CurrentRot, CameraResetTargetRotation, DeltaTime, CameraResetInterpSpeed);
                    PC->SetControlRotation(NewRot);

                    // 완료 체크
                    if (FMath::IsNearlyEqual(CurrentRot.Yaw, CameraResetTargetRotation.Yaw, 1.0f) &&
                        FMath::IsNearlyEqual(CurrentRot.Pitch, CameraResetTargetRotation.Pitch, 1.0f))
                    {
                        bIsResettingCamera = false;
                    }
                }
            }
        }
    }
}

void USFLockOnComponent::EnsureDefaultRotationMode()
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;
    
    UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
    if (!CMC) return;
    
    // 락온 중이 아닌데 CMC가 락온 모드면 복원
    if (!CurrentTarget && !CMC->bOrientRotationToMovement)
    {
        CMC->bOrientRotationToMovement = true;
        CMC->bUseControllerDesiredRotation = false;
    }
}

void USFLockOnComponent::ServerUpdate_RemoteClientRotation(float DeltaTime)
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    ACharacter* Character = Cast<ACharacter>(OwnerPawn);
    if (!Character) return;

    UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
    if (!CMC) return;

    // Dodge 중이면 스킵 (Dodge가 회전 제어)
    if (const IGameplayTagAssetInterface* TagInterface = Cast<const IGameplayTagAssetInterface>(OwnerPawn))
    {
        if (TagInterface->HasMatchingGameplayTag(SFGameplayTags::Ability_Hero_Dodge))
        {
            return;
        }
    }

    // Sprint 체크
    bool bIsSprinting = false;
    if (const IGameplayTagAssetInterface* TagInterface = Cast<const IGameplayTagAssetInterface>(OwnerPawn))
    {
        if (SprintTag.IsValid() && TagInterface->HasMatchingGameplayTag(SprintTag))
        {
            bIsSprinting = true;
        }
    }

    if (bIsSprinting)
    {
        CMC->bOrientRotationToMovement = true;
        CMC->bUseControllerDesiredRotation = false;
    }
    else
    {
        // 타겟 방향으로 직접 회전 (카메라 없이)
        CMC->bOrientRotationToMovement = false;
        CMC->bUseControllerDesiredRotation = false;  // 직접 제어

        FVector OwnerLoc = OwnerPawn->GetActorLocation();
        FVector TargetLoc = GetActorSocketLocation(CurrentTarget, CurrentTargetSocketName);
        FVector DirectionToTarget = (TargetLoc - OwnerLoc).GetSafeNormal2D();

        if (!DirectionToTarget.IsNearlyZero())
        {
            FRotator TargetRot = FRotator(0.0f, DirectionToTarget.Rotation().Yaw, 0.0f);
            FRotator SmoothRot = FMath::RInterpTo(Character->GetActorRotation(), TargetRot, DeltaTime, 15.0f);
            Character->SetActorRotation(SmoothRot);
        }
    }
}

// =========================================================
//  Logic: Server Authority (서버 전용 로직)
// =========================================================

void USFLockOnComponent::ServerUpdate_TargetValidation(float DeltaTime)
{
	if (!CurrentTarget) return;

	// 1. 타겟 완전 소멸 체크
	if (!IsValid(CurrentTarget))
	{
		Server_EndLockOn();
		return;
	}

	// 2. 거리 체크
	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), CurrentTarget->GetActorLocation());
	if (Distance > LockOnBreakDistance)
	{
		Server_EndLockOn();
		return;
	}

	// 3. 시야 가림 체크 (Grace Period 적용)
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(CurrentTarget);

	FVector Start = GetOwner()->GetActorLocation() + FVector(0, 0, 50);
	FVector End = GetActorSocketLocation(CurrentTarget, CurrentTargetSocketName);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult, Start, End, ECC_Visibility, QueryParams
	);

	if (bHit)
	{
		TimeSinceTargetHidden += DeltaTime;
		if (TimeSinceTargetHidden > LostTargetMemoryTime)
		{
			Server_EndLockOn();
		}
	}
	else
	{
		TimeSinceTargetHidden = 0.0f;
	}
}

void USFLockOnComponent::TryLockOn()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	// 로컬 쿨타임 체크 (스팸 방지)
	double CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastLockOnToggleTime < 0.2) return;
	LastLockOnToggleTime = CurrentTime;

	// 1. 이미 락온 중 -> 해제
	if (CurrentTarget)
	{
		EndLockOn(); // Server RPC
	}
	else
	{
		// 2. 락온 시도 -> 로컬에서 먼저 탐색 (Prediction)
		AActor* LocalBest = FindBestTarget();
		if (LocalBest)
		{
			// 타겟 발견: 서버로 요청
			Server_TryLockOn(LocalBest);
			bIsResettingCamera = false; // 리셋 중단
		}
		else
		{
			// 타겟 없음: 즉시 카메라 리셋 (서버 요청 X)
			ResetCamera();
		}
	}
}

void USFLockOnComponent::EndLockOn()
{
	Server_EndLockOn();
}

void USFLockOnComponent::ResetCamera()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled()) return;

	if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
	{
		// 캐릭터 정면 방향
		FRotator ActorRot = OwnerPawn->GetActorRotation();
		CameraResetTargetRotation = FRotator(-10.0f, ActorRot.Yaw, 0.0f); // Pitch는 약간 아래로
		bIsResettingCamera = true;
	}
}

void USFLockOnComponent::Server_TryLockOn_Implementation(AActor* TargetActor)
{
	// 이미 타겟이 있다면 해제 (Toggle 방식)
	if (CurrentTarget)
	{
		Server_EndLockOn();
		return;
	}

	if (IsTargetValid(TargetActor))
	{
		float Distance = FVector::Dist(GetOwner()->GetActorLocation(), TargetActor->GetActorLocation());
		
		if (Distance <= LockOnDistance * 1.2f) 
		{
			FName SocketName = DefaultLockOnSocketName;
			if (const ISFLockOnInterface* Interface = Cast<const ISFLockOnInterface>(TargetActor))
			{
				TArray<FName> Sockets = Interface->GetLockOnSockets();
				if (Sockets.Num() > 0) SocketName = Sockets[0];
			}

			Server_SetCurrentTarget(TargetActor, SocketName);
		}
	}
}

void USFLockOnComponent::Server_EndLockOn_Implementation()
{
	if (CurrentTarget)
	{
		Server_SetCurrentTarget(nullptr, NAME_None);
	}
}

void USFLockOnComponent::Server_SwitchTarget_Implementation(const FVector2D& Input)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !CurrentTarget) return;

	CurrentSwitchCooldown = SwitchCooldown;

	// 서버에서 카메라 방향을 정확히 알 수 없으므로, 현재 타겟을 기준으로 입력 방향을 추정하거나
	// 플레이어 컨트롤러 회전을 신뢰해야 함. 여기서는 간략히 컨트롤러 회전 사용.
	FRotator CamRot = OwnerPawn->GetControlRotation(); 
	FVector CamRight = CamRot.RotateVector(FVector::RightVector);
	FVector CamUp = CamRot.RotateVector(FVector::UpVector);
	
	FVector SearchDirection = (CamRight * Input.X + CamUp * Input.Y).GetSafeNormal();
	FVector CurrentLockLocation = GetActorSocketLocation(CurrentTarget, CurrentTargetSocketName);

	AActor* BestNewActor = nullptr;
	FName BestNewSocket = NAME_None;
	float ClosestDistSq = FLT_MAX; 

	// 주변 액터 검색
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		this, OwnerPawn->GetActorLocation(), LockOnDistance, ObjectTypes,
		AActor::StaticClass(), { OwnerPawn }, OverlappedActors
	);

	for (AActor* Candidate : OverlappedActors)
	{
		if (!IsTargetValid(Candidate)) continue;
		if (!IsHostile(Candidate)) continue;

		TArray<FName> CandidateSockets;
		if (const ISFLockOnInterface* Interface = Cast<const ISFLockOnInterface>(Candidate))
		{
			CandidateSockets = Interface->GetLockOnSockets();
		}
		if (CandidateSockets.Num() == 0) CandidateSockets.Add(DefaultLockOnSocketName);

		for (const FName& Socket : CandidateSockets)
		{
			if (Candidate == CurrentTarget && Socket == CurrentTargetSocketName) continue;

			FVector CandidateLoc = GetActorSocketLocation(Candidate, Socket);
			FVector DirToCand = (CandidateLoc - CurrentLockLocation).GetSafeNormal();

			float InputDot = FVector::DotProduct(SearchDirection, DirToCand);
			
			// 입력 방향과 일치하는 후보군 필터링
			if (InputDot > SwitchAngularLimit)
			{
				float DistSq = FVector::DistSquared(CurrentLockLocation, CandidateLoc);
				
				// 같은 대상의 다른 부위라면 우선순위 높임 (거리 보정)
				if (Candidate == CurrentTarget)
				{
					DistSq *= 0.5f; 
				}

				if (DistSq < ClosestDistSq)
				{
					ClosestDistSq = DistSq;
					BestNewActor = Candidate;
					BestNewSocket = Socket;
				}
			}
		}
	}

	if (BestNewActor)
	{
		Server_SetCurrentTarget(BestNewActor, BestNewSocket);
	}
}

void USFLockOnComponent::Server_SetCurrentTarget(AActor* NewTarget, FName SocketName)
{
	AActor* OldTarget = CurrentTarget;

	if (OldTarget == NewTarget && CurrentTargetSocketName == SocketName)
	{
		return;
	}

	// 1. 기존 타겟 이벤트 해제
	if (OldTarget)
	{
		UnregisterTargetEvents(OldTarget);
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
		{
			ASC->RemoveLooseGameplayTag(SFGameplayTags::Character_State_LockedOn);
		}
	}

	// 2. 값 변경
	CurrentTarget = NewTarget;
	CurrentTargetSocketName = SocketName;
	TimeSinceTargetHidden = 0.0f;
	bIsSwitchingTarget = false; 

	// 3. 새 타겟 이벤트 등록 및 태그 처리
	if (CurrentTarget)
	{
		RegisterTargetEvents(CurrentTarget);

		// Owner에게 LockOn 상태 태그 부여
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
		{
			ASC->AddLooseGameplayTag(SFGameplayTags::Character_State_LockedOn);
		}
	}

	// 4. 리슨 서버 호스트(Listen Server Host)를 위한 수동 OnRep 호출
	// (서버에서는 OnRep이 자동 호출되지 않으므로 직접 호출해야 함)
	OnRep_CurrentTarget(OldTarget);
}

// =========================================================
//  Logic: Event-Driven Death (핵심 개선)
// =========================================================

void USFLockOnComponent::RegisterTargetEvents(AActor* NewTarget)
{
	if (!NewTarget) return;

	// GAS Dead Tag 감지
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(NewTarget))
	{
		TargetDeathDelegateHandle = TargetASC->RegisterGameplayTagEvent(
			SFGameplayTags::Character_State_Dead,
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &USFLockOnComponent::OnTargetDeathTagChanged);
	}
	
	// Actor Destroy 감지 (비상용)
	// NewTarget->OnDestroyed.AddDynamic(this, ...); // 필요 시 추가
}

void USFLockOnComponent::UnregisterTargetEvents(AActor* OldTarget)
{
	if (!OldTarget) return;

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OldTarget))
	{
		if (TargetDeathDelegateHandle.IsValid())
		{
			TargetASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Dead, EGameplayTagEventType::NewOrRemoved).Remove(TargetDeathDelegateHandle);
			TargetDeathDelegateHandle.Reset();
		}
	}
}

void USFLockOnComponent::OnTargetDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	// Dead 태그가 추가되었다면(Count > 0) -> 즉시 반응
	if (NewCount > 0)
	{
		// [Auto-Switch Logic] 죽자마자 바로 다음 적 찾기
		AActor* NextTarget = FindBestTarget();
		if (NextTarget)
		{
			FName SocketName = DefaultLockOnSocketName;
			if (const ISFLockOnInterface* Interface = Cast<const ISFLockOnInterface>(NextTarget))
			{
				TArray<FName> Sockets = Interface->GetLockOnSockets();
				if (Sockets.Num() > 0) SocketName = Sockets[0];
			}
			Server_SetCurrentTarget(NextTarget, SocketName);
		}
		else
		{
			Server_SetCurrentTarget(nullptr, NAME_None);
		}
	}
}


// =========================================================
//  Logic: Client Presentation (클라이언트 전용)
// =========================================================

void USFLockOnComponent::OnRep_CurrentTarget(AActor* OldTarget)
{
	// 타겟 상태 변경에 따른 VFX 및 로컬 변수 처리
	if (CurrentTarget)
	{
		UpdateLockOnState(true);
		CreateLockOnEffect();
		bIsResettingCamera = false;
		
		APawn* OwnerPawn = Cast<APawn>(GetOwner());
		if (OwnerPawn && OwnerPawn->IsLocallyControlled())
		{
			// 타겟 스위칭 여부 판단 (기존 타겟이 있다가 바로 새 타겟이 된 경우)
			if (OldTarget != nullptr && OldTarget != CurrentTarget)
			{
				bIsSwitchingTarget = true;
			}
			else
			{
				bIsSwitchingTarget = false;
				// 처음 락온 시에는 현재 카메라 회전값에서 시작하도록 초기화
				if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
				{
					LastLockOnRotation = PC->GetControlRotation();
				}
			}
		}
	}
	else
	{
		UpdateLockOnState(false);
		DestroyLockOnEffect();
		bIsSwitchingTarget = false;
	}
}

void USFLockOnComponent::OnRep_CurrentTargetSocketName()
{
	UpdateLockOnEffectLocation();
}

void USFLockOnComponent::ClientUpdate_Rotation(float DeltaTime)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;

    // Dodge 중이면 스킵 (Dodge 어빌리티가 회전 관리)
    if (const IGameplayTagAssetInterface* TagInterface = Cast<const IGameplayTagAssetInterface>(OwnerPawn))
    {
        if (TagInterface->HasMatchingGameplayTag(SFGameplayTags::Ability_Hero_Dodge))
        {
            return; 
        }
    }
    
    APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
    if (!PC) return;

    // ========================================
    // 1. 카메라 회전 (타겟 추적)
    // ========================================
    FVector TargetLoc = GetActorSocketLocation(CurrentTarget, CurrentTargetSocketName);
    FVector CameraLoc = PC->PlayerCameraManager->GetCameraLocation();
    
    FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(CameraLoc, TargetLoc);

    // Pitch 제한 (Smart Pitch Clamp)
    float DistanceToTarget = FVector::Dist(OwnerPawn->GetActorLocation(), TargetLoc);
    float CurrentPitchMax = PitchLimitMax;

    if (DistanceToTarget < CloseRangeThreshold)
    {
        float Alpha = FMath::Clamp(DistanceToTarget / CloseRangeThreshold, 0.0f, 1.0f);
        CurrentPitchMax = FMath::Lerp(CloseRangePitchLimitMax, PitchLimitMax, Alpha);
    }

    LookAtRot.Pitch = FMath::Clamp(LookAtRot.Pitch, PitchLimitMin, CurrentPitchMax);

    // 보간
    float CurrentInterpSpeed = bIsSwitchingTarget ? TargetSwitchInterpSpeed : CameraInterpSpeed;
    FRotator SmoothRot = FMath::RInterpTo(LastLockOnRotation, LookAtRot, DeltaTime, CurrentInterpSpeed);

    PC->SetControlRotation(SmoothRot);
    LastLockOnRotation = SmoothRot;

    // 스위칭 완료 체크
    if (bIsSwitchingTarget)
    {
        FRotator Delta = (LookAtRot - LastLockOnRotation).GetNormalized();
        if (FMath::Abs(Delta.Yaw) < 2.0f && FMath::Abs(Delta.Pitch) < 2.0f)
        {
            bIsSwitchingTarget = false;
        }
    }

    // ========================================
    // 2. 캐릭터 회전 (Sprint 체크 포함)
    // ========================================
    bool bIsSprinting = false;
    if (const IGameplayTagAssetInterface* TagInterface = Cast<const IGameplayTagAssetInterface>(OwnerPawn))
    {
        if (SprintTag.IsValid() && TagInterface->HasMatchingGameplayTag(SprintTag))
        {
            bIsSprinting = true;
        }
    }

    ACharacter* Character = Cast<ACharacter>(OwnerPawn);
    if (Character)
    {
        UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
        
        if (bIsSprinting)
        {
            CMC->bOrientRotationToMovement = true;
            CMC->bUseControllerDesiredRotation = false;
        }
        else
        {
            // CMC에 회전 위임 (SetActorRotation 제거로 충돌 방지)
            CMC->bOrientRotationToMovement = false;
            CMC->bUseControllerDesiredRotation = true;
        }
    }
}

void USFLockOnComponent::ClientUpdate_SwitchingInput(float DeltaTime)
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

	float InputX = 0.0f;
	float InputY = 0.0f;
	PC->GetInputAnalogStickState(EControllerAnalogStick::CAS_RightStick, InputX, InputY);

	// 마우스 입력 추가 (Sensitivity)
	if (FMath::IsNearlyZero(InputX) && FMath::IsNearlyZero(InputY))
	{
		float MouseX = 0.0f, MouseY = 0.0f;
		PC->GetInputMouseDelta(MouseX, MouseY);
		InputX = MouseX * 0.3f; 
		InputY = MouseY * 0.3f;
	}

	FVector2D CurrentInput(InputX, InputY);
	if (CurrentInput.Size() >= SwitchInputThreshold)
	{
		Server_SwitchTarget(CurrentInput);
		CurrentSwitchCooldown = SwitchCooldown;
	}
}

void USFLockOnComponent::UpdateLockOnState(bool bIsLockedOn)
{
	// 1. 캐릭터 회전 모드 변경 
	UpdateCharacterRotationMode(bIsLockedOn);

	// 2. GAS 태그 클라이언트 동기화
	// 서버뿐만 아니라 클라이언트도 이 태그를 가지고 있어야 AnimBP가 확실하게 반응합니다.
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		if (bIsLockedOn)
		{
			ASC->AddLooseGameplayTag(SFGameplayTags::Character_State_LockedOn);
		}
		else
		{
			ASC->RemoveLooseGameplayTag(SFGameplayTags::Character_State_LockedOn);
		}
	}
}

void USFLockOnComponent::UpdateCharacterRotationMode(bool bLockOnEnabled)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character) return;

	UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
	if (!CMC) return;

	if (bLockOnEnabled)
	{
		// 락온 켜짐: 캐릭터가 이동 방향으로 회전하지 않도록 설정 (Strafing 준비)
		CMC->bOrientRotationToMovement = false;
		
		// 카메라는 우리가 직접 제어하므로 컨트롤러 회전을 자동으로 따르지 않게 함
		CMC->bUseControllerDesiredRotation = true; 
	}
	else
	{
		// 락온 꺼짐: 다시 이동 방향으로 캐릭터가 회전하도록 복구
		CMC->bOrientRotationToMovement = true;
		CMC->bUseControllerDesiredRotation = false;
		
		// 혹시 모를 Yaw 고정도 해제
		Character->bUseControllerRotationYaw = false;
	}
}

// =========================================================
//  Helpers & VFX
// =========================================================

AActor* USFLockOnComponent::FindBestTarget()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return nullptr;
	
	// 서버에서 Controller 구하기
	AController* Controller = OwnerPawn->GetController();
	FVector CameraLoc; 
	FRotator CameraRot;

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->GetPlayerViewPoint(CameraLoc, CameraRot);
	}
	else
	{
		CameraLoc = OwnerPawn->GetActorLocation();
		CameraRot = OwnerPawn->GetControlRotation();
	}
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

		FVector DirToTarget = (Candidate->GetActorLocation() - CameraLoc).GetSafeNormal();
		float DotResult = FVector::DotProduct(CameraForward, DirToTarget);

		if (DotResult < ScreenCenterWeight) continue;

		float Score = 0.0f;

		// [A] 거리 점수
		float Distance = FVector::Dist(OwnerPawn->GetActorLocation(), Candidate->GetActorLocation());
		float DistScore = FMath::Clamp(1.0f - (Distance / LockOnDistance), 0.0f, 1.0f);
		Score += DistScore * Weight_Distance;

		// [B] 각도 점수
		float AngleScore = (DotResult - ScreenCenterWeight) / (1.0f - ScreenCenterWeight);
		Score += AngleScore * Weight_Angle;

		// [C] 보스 보너스
		if (const IGameplayTagAssetInterface* TagInterface = Cast<const IGameplayTagAssetInterface>(Candidate))
		{
			if (TagInterface->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Character.Type.Boss"), false)))
			{
				Score += Weight_BossBonus;
			}
		}

		// [D] 가시성 검사
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(OwnerPawn);
		Params.AddIgnoredActor(Candidate);
		
		FVector TestTargetLoc = GetActorSocketLocation(Candidate, NAME_None);
		bool bVisible = !GetWorld()->LineTraceSingleByChannel(
			Hit, CameraLoc, TestTargetLoc, ECC_Visibility, Params
		);

		if (!bVisible)
		{
			Score *= 0.5f; 
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

void USFLockOnComponent::CreateLockOnEffect()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && !OwnerPawn->IsLocallyControlled()) return; // 로컬만 생성
	
	DestroyLockOnEffect();

	if (!CurrentTarget || !LockOnEffectTemplate) return;

	USceneComponent* TargetMesh = CurrentTarget->FindComponentByClass<USceneComponent>();
	if (ACharacter* TargetChar = Cast<ACharacter>(CurrentTarget))
	{
		TargetMesh = TargetChar->GetMesh();
	}
	
	if (TargetMesh)
	{
		LockOnEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			LockOnEffectTemplate,
			TargetMesh,
			CurrentTargetSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);
	}
}

void USFLockOnComponent::DestroyLockOnEffect()
{
	if (LockOnEffectComponent)
	{
		LockOnEffectComponent->DestroyComponent();
		LockOnEffectComponent = nullptr;
	}
}

void USFLockOnComponent::UpdateLockOnEffectLocation()
{
	if (LockOnEffectComponent && CurrentTarget)
	{
		USceneComponent* TargetMesh = CurrentTarget->FindComponentByClass<USceneComponent>();
		if (ACharacter* TargetChar = Cast<ACharacter>(CurrentTarget))
		{
			TargetMesh = TargetChar->GetMesh();
		}
		
		if (TargetMesh)
		{
			LockOnEffectComponent->AttachToComponent(
				TargetMesh, 
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, 
				CurrentTargetSocketName
			);
		}
	}
}

bool USFLockOnComponent::IsTargetValid(AActor* TargetActor) const
{
	if (!TargetActor) return false;
	
	// 1. Interface Check
	if (const ISFLockOnInterface* LockOnTarget = Cast<const ISFLockOnInterface>(TargetActor))
	{
		if (!LockOnTarget->CanBeLockedOn()) return false;
	}
	
	// 2. Dead Tag Check (Interface가 없거나, Interface에서 true여도 태그로 재확인)
	if (const IGameplayTagAssetInterface* TagInterface = Cast<const IGameplayTagAssetInterface>(TargetActor))
	{
		if (TagInterface->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
		{
			return false;
		}
	}
	
	return true;
}

bool USFLockOnComponent::IsHostile(AActor* TargetActor) const
{
	// 1. Enemy 클래스면 적으로 간주 (가장 확실)
	if (TargetActor->IsA<ASFEnemy>())
	{
		return true;
	}
    
	// 2. GameplayTag 체크 (fallback)
	if (TargetTags.Num() > 0)
	{
		if (const IGameplayTagAssetInterface* TagInterface = Cast<const IGameplayTagAssetInterface>(TargetActor))
		{
			FGameplayTagContainer OwnedTags;
			TagInterface->GetOwnedGameplayTags(OwnedTags);
            
			if (OwnedTags.HasAny(TargetTags))
			{
				return true;
			}
		}
	}
    
	// 2. Team 시스템 (fallback)
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return false;

	const IGenericTeamAgentInterface* OwnerTeamAgent = Cast<const IGenericTeamAgentInterface>(OwnerPawn);
	const IGenericTeamAgentInterface* TargetTeamAgent = Cast<const IGenericTeamAgentInterface>(TargetActor);

	if (OwnerTeamAgent && TargetTeamAgent)
	{
		return OwnerTeamAgent->GetTeamAttitudeTowards(*TargetActor) == ETeamAttitude::Hostile;
	}

	return false;
}

FVector USFLockOnComponent::GetActorSocketLocation(AActor* Actor, FName SocketName) const
{
	if (!Actor) return FVector::ZeroVector;
	
	if (SocketName != NAME_None)
	{
		USceneComponent* TargetMesh = nullptr;
		if (ACharacter* CharActor = Cast<ACharacter>(Actor))
		{
			TargetMesh = CharActor->GetMesh();
		}
		else
		{
			TargetMesh = Actor->FindComponentByClass<USceneComponent>();
		}

		if (TargetMesh && TargetMesh->DoesSocketExist(SocketName))
		{
			return TargetMesh->GetSocketLocation(SocketName);
		}
	}
	return Actor->GetActorLocation();
}