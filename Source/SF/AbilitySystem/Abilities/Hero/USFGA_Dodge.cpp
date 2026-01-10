#include "USFGA_Dodge.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/SFCharacterBase.h"
#include "MotionWarpingComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/GameplayEffect/Hero/EffectContext/SFTargetDataTypes.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"

USFGA_Dodge::USFGA_Dodge(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 예측 정책 설정
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	SetAssetTags(FGameplayTagContainer(SFGameplayTags::Ability_Hero_Dodge));
}

void USFGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitCheck(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 공중 사용 불가 체크
	UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
	if (CMC->IsFalling())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 구르기 중에는 CMC가 멋대로 캐릭터를 회전시키지 못하게 막습니다.
	// 기존 설정 백업
	bSavedOrientRotationToMovement = CMC->bOrientRotationToMovement;
	bSavedUseControllerRotationYaw = Character->bUseControllerRotationYaw;

	// 회전 제어 비활성화 (루트모션과 모션워핑이 전적으로 제어하도록)
	CMC->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = false;

	// 2. 락온 상태 확인
	bool bIsLockedOn = Character->HasMatchingGameplayTag(SFGameplayTags::Character_State_LockedOn);

	// 3. 클라이언트(혹은 Standalone) 처리
	if (IsLocallyControlled())
	{
		FVector TargetLoc;
		FRotator TargetRot;
		ESFDodgeDirection DodgeDir;

		CalculateDodgeParameters(bIsLockedOn, TargetLoc, TargetRot, DodgeDir);
		ApplyDodge(TargetLoc, TargetRot, DodgeDir);

		USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
		if (ASC && !Character->HasAuthority())
		{
			FGameplayAbilityTargetData_LocationInfo* NewData = new FGameplayAbilityTargetData_LocationInfo();
			NewData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
			
			FVector PackedParams((float)DodgeDir, 1.0f, 1.0f); 
			NewData->TargetLocation.LiteralTransform = FTransform(TargetRot, TargetLoc, PackedParams);

			FGameplayAbilityTargetDataHandle HandleData(NewData);
			
			ASC->ServerSetReplicatedTargetData(
				CurrentSpecHandle, 
				CurrentActivationInfo.GetActivationPredictionKey(), 
				HandleData, 
				FGameplayTag(), 
				ASC->ScopedPredictionKey
			);
		}
	}
	// 4. 서버 처리
	else
	{
		USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
		if (ASC)
		{
			const FPredictionKey ActivationPredictionKey = CurrentActivationInfo.GetActivationPredictionKey();

			FAbilityTargetDataSetDelegate& TargetDataDelegate = ASC->AbilityTargetDataSetDelegate(
				CurrentSpecHandle, 
				ActivationPredictionKey
			);
			
			ServerTargetDataDelegateHandle = TargetDataDelegate.AddUObject(this, &USFGA_Dodge::OnServerTargetDataReceived);
			ASC->CallReplicatedTargetDataDelegatesIfSet(CurrentSpecHandle, ActivationPredictionKey);
		}
	}

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
}

void USFGA_Dodge::CalculateDodgeParameters(bool bIsLockedOn, FVector& OutLocation, FRotator& OutRotation, ESFDodgeDirection& OutDirection) const
{
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (!Character) return;

	// 1. 입력 방향 가져오기
	FVector InputDir = Character->GetLastInputDirection(); 
	bool bHasInput = !InputDir.IsNearlyZero();

	// 2. 입력이 없으면 백스텝
	if (!bHasInput)
	{
		OutRotation = Character->GetActorRotation();
		OutLocation = Character->GetActorLocation() - (Character->GetActorForwardVector() * DodgeDistance * 0.5f);
		OutDirection = ESFDodgeDirection::Backstep;
		return;
	}

	// 3. 입력이 있을 때
	FRotator ControlRot = Character->GetControlRotation();
	ControlRot.Pitch = 0.0f;
	ControlRot.Roll = 0.0f;

	if (bIsLockedOn)
	{
		// [Locked On] 
		OutRotation = ControlRot; 
		OutLocation = Character->GetActorLocation() + (InputDir.GetSafeNormal() * DodgeDistance);

		FVector LocalDir = ControlRot.UnrotateVector(InputDir);
		
		// 락온 상태에서도 부드럽게 나가도록 Threshold 조정 가능
		if (FMath::Abs(LocalDir.X) > FMath::Abs(LocalDir.Y))
		{
			OutDirection = (LocalDir.X > 0) ? ESFDodgeDirection::Forward : ESFDodgeDirection::Backward;
		}
		else
		{
			OutDirection = (LocalDir.Y > 0) ? ESFDodgeDirection::Right : ESFDodgeDirection::Left;
		}
	}
	else
	{
		// [Free Camera]
		OutRotation = InputDir.Rotation(); // 입력 방향을 봄
		OutRotation.Pitch = 0.0f;
		OutLocation = Character->GetActorLocation() + (InputDir.GetSafeNormal() * DodgeDistance);
		
		// 프리 카메라 모드에서는 항상 앞구르기 모션 사용
		OutDirection = ESFDodgeDirection::Forward;
	}
}

UAnimMontage* USFGA_Dodge::GetMontageFromDirection(ESFDodgeDirection Direction) const
{
	switch (Direction)
	{
	case ESFDodgeDirection::Forward:  return DirectionalDodgeMontages.ForwardMontage;
	case ESFDodgeDirection::Backward: return DirectionalDodgeMontages.BackwardMontage;
	case ESFDodgeDirection::Left:     return DirectionalDodgeMontages.LeftMontage;
	case ESFDodgeDirection::Right:    return DirectionalDodgeMontages.RightMontage;
	case ESFDodgeDirection::Backstep: return BackstepMontage;
	default: return DirectionalDodgeMontages.ForwardMontage;
	}
}

void USFGA_Dodge::ApplyDodge(const FVector& TargetLocation, const FRotator& TargetRotation, ESFDodgeDirection Direction)
{
	// 1. Motion Warping 설정 (몽타주 재생 전 필수)
	SetupMotionWarping(TargetLocation, TargetRotation);

	// 2. 몽타주 재생
	UAnimMontage* MontageToPlay = GetMontageFromDirection(Direction);

	if (MontageToPlay)
	{
		UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, 
			NAME_None, 
			MontageToPlay, 
			1.0f, 
			NAME_None, 
			false, 
			1.0f
		);
		
		if (Task)
		{
			Task->OnBlendOut.AddDynamic(this, &USFGA_Dodge::OnMontageFinished);
			Task->OnInterrupted.AddDynamic(this, &USFGA_Dodge::OnMontageFinished);
			Task->OnCancelled.AddDynamic(this, &USFGA_Dodge::OnMontageFinished);
			Task->OnCompleted.AddDynamic(this, &USFGA_Dodge::OnMontageFinished);
			Task->ReadyForActivation();
		}
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void USFGA_Dodge::OnServerTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
{
	// [Server] 클라이언트가 보낸 데이터를 신뢰하여 실행
	
	const FGameplayAbilityTargetData_LocationInfo* LocationData = static_cast<const FGameplayAbilityTargetData_LocationInfo*>(DataHandle.Get(0));
	if (LocationData)
	{
		// 데이터 언패킹 (Unpacking)
		FTransform ReceivedTransform = LocationData->TargetLocation.LiteralTransform;
		
		FVector TargetWarpLoc = ReceivedTransform.GetLocation();
		FRotator TargetFaceRot = ReceivedTransform.Rotator();
		
		// Scale.X에 숨겨둔 Enum 값을 복원 (float -> uint8 -> Enum)
		uint8 EnumIndex = (uint8)ReceivedTransform.GetScale3D().X;
		ESFDodgeDirection ReceivedDirection = (ESFDodgeDirection)EnumIndex;

		// GAS 시스템에 "타겟 데이터 사용함" 알림
		USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
		if (ASC)
		{
			ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
		}

		// 서버 실행 (클라이언트와 완벽히 동일한 동작 보장)
		ApplyDodge(TargetWarpLoc, TargetFaceRot, ReceivedDirection);
	}
	else
	{
		// 데이터 오류 시 강제 종료
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void USFGA_Dodge::SetupMotionWarping(const FVector& TargetLocation, const FRotator& TargetRotation)
{
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (Character)
	{
		if (UMotionWarpingComponent* MW = Character->GetMotionWarpingComponent())
		{
			MW->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, TargetRotation);
		}
	}
}

void USFGA_Dodge::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (Character)
	{
		UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
		if (CMC)
		{
			CMC->bOrientRotationToMovement = bSavedOrientRotationToMovement;
		}
		Character->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;

		// Motion Warping 타겟 정리
		if (UMotionWarpingComponent* MW = Character->GetMotionWarpingComponent())
		{
			MW->RemoveWarpTarget(WarpTargetName);
		}
	}
	
	if (ActorInfo->IsNetAuthority() && ServerTargetDataDelegateHandle.IsValid())
	{
		USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
		if (ASC)
		{
			FAbilityTargetDataSetDelegate& TargetDataDelegate = ASC->AbilityTargetDataSetDelegate(
				Handle, 
				ActivationInfo.GetActivationPredictionKey()
			);
			TargetDataDelegate.Remove(ServerTargetDataDelegateHandle);
		}
		ServerTargetDataDelegateHandle.Reset();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}