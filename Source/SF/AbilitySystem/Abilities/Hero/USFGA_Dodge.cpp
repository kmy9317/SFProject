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
	SetAssetTags(FGameplayTagContainer(SFGameplayTags::Ability_Hero_Dodge));
}

void USFGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 공중 사용 불가 체크
	if (Character->GetCharacterMovement()->IsFalling())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. 락온 상태 확인 (GAS 태그)
	bool bIsLockedOn = Character->HasMatchingGameplayTag(SFGameplayTags::Character_State_LockedOn);

	if (IsLocallyControlled())
	{
		// [Client/Standalone] 예측 실행
		FVector TargetLoc;
		FRotator TargetRot;
		UAnimMontage* MontageToPlay = nullptr;

		// 파라미터 및 몽타주 계산
		CalculateDodgeParameters(bIsLockedOn, TargetLoc, TargetRot, MontageToPlay);

		// 실행
		ApplyDodge(TargetLoc, TargetRot, MontageToPlay);

		// 서버로 타겟 데이터 전송
		USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
		if (ASC && !Character->HasAuthority())
		{
			FGameplayAbilityTargetData_LocationInfo* NewData = new FGameplayAbilityTargetData_LocationInfo();
			NewData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
			NewData->TargetLocation.LiteralTransform = FTransform(TargetRot, TargetLoc);

			FGameplayAbilityTargetDataHandle HandleData(NewData);
			ASC->ServerSetReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey(), HandleData, FGameplayTag(), ASC->ScopedPredictionKey);
		}
	}
	else
	{
		// [Server] 클라이언트 데이터 대기
		USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
		if (ASC)
		{
			FAbilityTargetDataSetDelegate& TargetDataDelegate = ASC->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
			ServerTargetDataDelegateHandle = TargetDataDelegate.AddUObject(this, &USFGA_Dodge::OnServerTargetDataReceived);
		}
	}
}

void USFGA_Dodge::CalculateDodgeParameters(bool bIsLockedOn, FVector& OutLocation, FRotator& OutRotation, UAnimMontage*& OutMontage) const
{
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (!Character) return;

	// 1. 입력 방향 가져오기
	FVector InputDir = Character->GetLastInputDirection(); // 월드 기준 입력 벡터
	bool bHasInput = !InputDir.IsNearlyZero();

	// 2. 입력이 없으면 백스텝
	if (!bHasInput)
	{
		// 백스텝은 현재 방향 유지 + 뒤로 이동
		OutRotation = Character->GetActorRotation();
		OutLocation = Character->GetActorLocation() - (Character->GetActorForwardVector() * DodgeDistance * 0.5f); // 백스텝은 거리를 좀 짧게
		OutMontage = BackstepMontage;
		return;
	}

	// 3. 입력이 있을 때 (락온 여부에 따른 분기)
	FRotator ControlRot = Character->GetControlRotation();
	ControlRot.Pitch = 0.0f;
	ControlRot.Roll = 0.0f;

	if (bIsLockedOn)
	{
		// [Locked On] 
		// 회전: 타겟(카메라) 방향 고정
		OutRotation = ControlRot;

		// 몽타주: 입력 방향에 따라 4방향 선택 (전/후/좌/우)
		OutMontage = SelectMontageBasedOnInput(InputDir, ControlRot);

		// 위치: 입력 방향으로 이동
		OutLocation = Character->GetActorLocation() + (InputDir.GetSafeNormal() * DodgeDistance);
	}
	else
	{
		// [Free Camera]
		// 회전: 입력 방향을 바라봄
		OutRotation = InputDir.Rotation();
		OutRotation.Pitch = 0.0f;

		// 몽타주: 무조건 앞구르기
		OutMontage = DirectionalDodgeMontages.ForwardMontage;

		// 위치: 입력 방향으로 이동
		OutLocation = Character->GetActorLocation() + (InputDir.GetSafeNormal() * DodgeDistance);
	}
}

UAnimMontage* USFGA_Dodge::SelectMontageBasedOnInput(const FVector& InputDirection, const FRotator& ControlRotation) const
{
	// 입력 벡터를 컨트롤러(카메라) 기준 로컬 공간으로 변환
	FVector LocalDir = ControlRotation.UnrotateVector(InputDirection);
	
	// 각도에 따른 4방향 판정
	// X+: Forward, X-: Backward, Y+: Right, Y-: Left
	
	float AbsX = FMath::Abs(LocalDir.X);
	float AbsY = FMath::Abs(LocalDir.Y);

	if (AbsX > AbsY) // 앞/뒤 움직임이 더 큼
	{
		return (LocalDir.X > 0) ? DirectionalDodgeMontages.ForwardMontage : DirectionalDodgeMontages.BackwardMontage;
	}
	else // 좌/우 움직임이 더 큼
	{
		return (LocalDir.Y > 0) ? DirectionalDodgeMontages.RightMontage : DirectionalDodgeMontages.LeftMontage;
	}
}

void USFGA_Dodge::ApplyDodge(const FVector& TargetLocation, const FRotator& TargetRotation, UAnimMontage* MontageToPlay)
{
	// 1. Motion Warping 설정
	SetupMotionWarping(TargetLocation, TargetRotation);

	// 2. 몽타주 재생
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
		// 몽타주가 없으면 즉시 종료 (안전장치)
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void USFGA_Dodge::OnServerTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
{
	// 서버에서 클라이언트의 예측 데이터를 받아 실행
	const FGameplayAbilityTargetData_LocationInfo* LocationData = static_cast<const FGameplayAbilityTargetData_LocationInfo*>(DataHandle.Get(0));
	if (LocationData)
	{
		FTransform TargetTransform = LocationData->TargetLocation.LiteralTransform;
		
		// 몽타주 재계산 (서버에서도 동일한 입력/락온 상태라고 가정하거나, 클라에서 몽타주 인덱스를 보내는게 더 확실하지만,
		// 여기서는 로컬 상태 기준으로 다시 계산. 더 정확히 하려면 TargetData에 몽타주 정보도 포함해야 함)
		// *개선*: 여기서는 편의상 서버에서도 Calculate를 다시 호출하여 몽타주를 결정합니다.
		// (동기화 문제가 발생한다면 TargetData를 커스텀하여 몽타주 ID를 보내야 합니다)
		
		ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
		bool bIsLockedOn = Character && Character->HasMatchingGameplayTag(SFGameplayTags::Character_State_LockedOn);
		
		// 위치/회전은 클라에서 받은걸 신뢰, 몽타주는 서버 상태 기준으로 선택
		FVector DummyLoc; FRotator DummyRot; UAnimMontage* MontageToPlay = nullptr;
		CalculateDodgeParameters(bIsLockedOn, DummyLoc, DummyRot, MontageToPlay);

		ApplyDodge(TargetTransform.GetLocation(), TargetTransform.Rotator(), MontageToPlay);
	}
	else
	{
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
	// 델리게이트 정리
	if (ActorInfo->IsNetAuthority() && ServerTargetDataDelegateHandle.IsValid())
	{
		USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			FAbilityTargetDataSetDelegate& TargetDataDelegate = ASC->AbilityTargetDataSetDelegate(
				CurrentSpecHandle, 
				CurrentActivationInfo.GetActivationPredictionKey()
			);
			TargetDataDelegate.Remove(ServerTargetDataDelegateHandle);
		}
		ServerTargetDataDelegateHandle.Reset();
	}

	// Motion Warping 타겟 정리
	if (ASFCharacterBase* Character = GetSFCharacterFromActorInfo())
	{
		if (UMotionWarpingComponent* MW = Character->GetMotionWarpingComponent())
		{
			MW->RemoveWarpTarget(WarpTargetName);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}