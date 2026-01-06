// Fill out your copyright notice in the Description page of Project Settings.


#include "USFGA_Dodge.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/SFCharacterBase.h"
#include "MotionWarpingComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/GameplayEffect/Hero/EffectContext/SFTargetDataTypes.h"
#include "Character/SFCharacterGameplayTags.h"

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

	// 1. [순서 변경] 가능 여부 먼저 판단 (공중 불가)
	if (Character->GetCharacterMovement()->IsFalling())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. [순서 변경] 비용(스태미나) 먼저 지불
	// 실패하면 아무 일도 일어나지 않아야 함 (달리기가 끊기면 안 됨)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. [로직 수행] 비용 지불 성공 후, 상충되는 어빌리티(달리기) 취소
	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		FGameplayTagContainer CancelTags;
		CancelTags.AddTag(SFGameplayTags::Character_State_Sprint); 
		ASC->CancelAbilities(&CancelTags, nullptr, this);
	}
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// =================================================================
	// 클라이언트와 서버의 로직 분기
	// =================================================================

	if (IsLocallyControlled())
	{
		// --- [Local Client / Host] ---
		
		// 1. 방향 및 위치 계산
		FVector TargetLocation;
		FRotator TargetRotation;
		CalculateDodgeParameters(TargetLocation, TargetRotation);

		// 2. 서버로 보낼 데이터 패키징
		FScopedPredictionWindow ScopedPrediction(GetAbilitySystemComponentFromActorInfo());
		
		FSFGameplayAbilityTargetData_ChargePhase* NewData = new FSFGameplayAbilityTargetData_ChargePhase();
		NewData->RushTargetLocation = TargetLocation; // 위치 저장
		NewData->RushTargetRotation = TargetRotation; // 회전 저장
		NewData->PhaseIndex = 0; // 구르기는 페이즈 없으므로 0

		FGameplayAbilityTargetDataHandle DataHandle(NewData);

		// 3. 서버로 전송
		GetAbilitySystemComponentFromActorInfo()->ServerSetReplicatedTargetData(
				GetCurrentAbilitySpecHandle(), 
				GetCurrentActivationInfo().GetActivationPredictionKey(), 
				DataHandle, 
				FGameplayTag(), 
				GetAbilitySystemComponentFromActorInfo()->ScopedPredictionKey);

		// 4. 로컬(내 화면)에서 즉시 실행 (예측)
		ApplyDodge(TargetLocation, TargetRotation);
	}
	else if (ActorInfo->IsNetAuthority())
	{
		// --- [Dedicated Server / Remote Server] ---

		// 1. 클라이언트가 보낸 데이터 수신 대기
		USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			FAbilityTargetDataSetDelegate& TargetDataDelegate = ASC->AbilityTargetDataSetDelegate(
				CurrentSpecHandle, 
				CurrentActivationInfo.GetActivationPredictionKey()
			);

			// 데이터가 오면 OnServerTargetDataReceived 함수 실행
			ServerTargetDataDelegateHandle = TargetDataDelegate.AddUObject(this, &ThisClass::OnServerTargetDataReceived);

			// 혹시 Activate보다 데이터가 먼저 도착했는지 확인
			ASC->CallReplicatedTargetDataDelegatesIfSet(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
		}
	}
}

void USFGA_Dodge::OnServerTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
{
	// 데이터 사용 처리 (메모리 해제 등)
	GetAbilitySystemComponentFromActorInfo()->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	
	// 데이터 꺼내기
	const FSFGameplayAbilityTargetData_ChargePhase* ReceivedData = static_cast<const FSFGameplayAbilityTargetData_ChargePhase*>(DataHandle.Get(0));
	
	if (ReceivedData)
	{
		// 서버는 계산하지 않고, 클라이언트가 준 위치대로 이동
		ApplyDodge(ReceivedData->RushTargetLocation, ReceivedData->RushTargetRotation);
	}
}

void USFGA_Dodge::ApplyDodge(const FVector& TargetLocation, const FRotator& TargetRotation)
{
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (!Character) return;

	// 1. Motion Warping 설정
	SetupMotionWarping(TargetLocation, TargetRotation);

	// 2. 캐릭터 회전 강제 설정
	Character->SetActorRotation(TargetRotation);

	// 3. 몽타주 재생
	if (DodgeMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			TEXT("DodgeMontage"),
			DodgeMontage,
			1.f,
			NAME_None,
			true // StopWhenAbilityEnds
		);

		if (MontageTask)
		{
			MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
			MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
			MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
			MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageFinished);
			MontageTask->ReadyForActivation();
		}
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void USFGA_Dodge::CalculateDodgeParameters(FVector& OutLocation, FRotator& OutRotation) const
{
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (!Character) return;

	// 플레이어의 입력 방향 가져오기
	FVector InputDir = Character->GetLastMovementInputVector();

	// 입력 없으면 백스텝
	if (InputDir.IsNearlyZero())
	{
		InputDir = -Character->GetActorForwardVector();
	}
	else
	{
		InputDir.Normalize();
	}

	OutRotation = InputDir.Rotation();

	FVector StartParams = Character->GetActorLocation();
	FVector EndParams = StartParams + (InputDir * DodgeDistance);

	// 벽 뚫기 방지
	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, StartParams, EndParams, ECC_Visibility);

	if (HitResult.bBlockingHit)
	{
		OutLocation = HitResult.Location - (InputDir * 30.f);
	}
	else
	{
		OutLocation = EndParams;
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
	// 서버 델리게이트 정리
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