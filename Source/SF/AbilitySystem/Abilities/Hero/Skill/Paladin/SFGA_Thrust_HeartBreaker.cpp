// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGA_Thrust_HeartBreaker.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/SFCharacterBase.h"
#include "AbilitySystem/GameplayEffect/Hero/EffectContext/SFTargetDataTypes.h"
#include "AbilitySystem/Tasks/Combat/SFAbilityTask_UpdateWarpTarget.h"
#include "Camera/SFCameraMode.h"
#include "Character/Hero/SFHeroComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Messages/SFMessageGameplayTags.h"
#include "Messages/SFSkillInfoMessages.h"
#include "Player/SFPlayerController.h"
#include "Weapons/Actor/SFEquipmentBase.h"

USFGA_Thrust_HeartBreaker::USFGA_Thrust_HeartBreaker(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseWindupWarp = true;
	bUseEquipmentWarpSettings = false;
	
	WarpTargetNameOverride = TEXT("AttackTarget");
	WarpRangeOverride = 124.f;  
	RotationInterpSpeedOverride = 1.f;
	MaxWindupTurnAngleOverride = 180.f;
}

void USFGA_Thrust_HeartBreaker::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (PhaseInfos.IsValidIndex(0))
	{
		WarpRangeOverride = BaseRushDistance * PhaseInfos[0].RushDistanceScale;
	}
	else
	{
		WarpRangeOverride = BaseRushDistance;
	}

	RotationInterpSpeedOverride = ChargingInterpSpeed;
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AbilityStartTime = GetWorld()->GetTimeSeconds();
	MaxPhaseIndex = PhaseInfos.Num() > 0 ? PhaseInfos.Num() - 1 : 0;
	
	ResetCharge();
	
	// 슈퍼아머 적용
    if (SuperArmorEffectClass)
    {
        FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(SuperArmorEffectClass);
        if (SpecHandle.IsValid())
        {
            SuperArmorEffectHandle = ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
        }
    }

	// 차징 몽타주 재생 (Start → Loop)
	if (ChargingMontage)
	{
		UAbilityTask_PlayMontageAndWait* ChargingMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("Charging"), ChargingMontage, 1.f, TEXT("Start"), false
		);
		ChargingMontageTask->ReadyForActivation();
	}
	
	if (ASFPlayerController* SFPlayerController = GetSFPlayerControllerFromActorInfo())
	{
		SFPlayerController->SetIgnoreMoveInput(true);
	}

	// 로컬 클라이언트 일때만 입력 릴리즈 대기
	if (ActorInfo->IsLocallyControlled())
	{
		// 입력 릴리즈 대기
		if (UAbilityTask_WaitInputRelease* WaitInputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true))
		{
			WaitInputReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnKeyReleased);
			WaitInputReleaseTask->ReadyForActivation();
		}
		
		// TotalChargeTime 계산 (마지막 Phase 제외)
		TotalChargeTime = 0.f;
		for (int32 i = 0; i < PhaseInfos.Num() - 1; ++i)
		{
			TotalChargeTime += PhaseInfos[i].ChargeTimeToNext;
		}
		// UI 표시
		BroadcastUIConstruct(true);
	}

	StartChargingCue();
	StartPhaseTimer();

	if (GetPhaseCameraMode())
	{
		UpdateCameraModeForPhase(0);
	}
	else
	{
		SetCameraMode(CameraModeClass);
	}

	// 서버인 경우 클라이언트로부터 TargetData(차지 페이즈, 위치) 수신 대기
	if (ActorInfo->IsNetAuthority())
	{
		USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			// 해당 SpecHandle과 PredictionKey에 맞는 TargetData가 오면 호출될 델리게이트 가져오기
			FAbilityTargetDataSetDelegate& TargetDataDelegate = ASC->AbilityTargetDataSetDelegate(
				CurrentSpecHandle, 
				CurrentActivationInfo.GetActivationPredictionKey()
			);

			// 콜백 바인딩 
			ServerTargetDataDelegateHandle = TargetDataDelegate.AddUObject(this, &ThisClass::OnServerTargetDataReceivedCallback);

			// 바인딩하기 전에 데이터가 이미 도착했는지 확인하고 처리
			// 네트워크 레이턴시나 패킷 순서에 따라 Activate보다 데이터가 먼저 도착했을 수도 있음
			ASC->CallReplicatedTargetDataDelegatesIfSet(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
		}
	}
}

void USFGA_Thrust_HeartBreaker::StartPhaseTimer()
{
	// 마지막 페이즈라면 타이머를 돌리지 않고 대기 (키 뗄 때까지 Max 유지)
	if (CurrentPhaseIndex >= MaxPhaseIndex)
	{
		// 풀차지 이펙트/사운드 재생 등을 여기서 처리 가능
		return;
	}
	
	if (PhaseInfos.IsValidIndex(CurrentPhaseIndex))
	{
		float PhaseTime = PhaseInfos[CurrentPhaseIndex].ChargeTimeToNext;
		// TODO: 공격 속도 적용 필요 시 여기서 계산(UI 표시)
		// PhaseTime /= GetAttackSpeed();
		GetWorld()->GetTimerManager().SetTimer(
			PhaseTimerHandle,
			this,
			&ThisClass::OnPhaseTimePassed,
			PhaseTime,
			false
		);
	}
}

void USFGA_Thrust_HeartBreaker::OnPhaseTimePassed()
{
	CurrentPhaseIndex++;

	if (CurrentPhaseIndex > MaxPhaseIndex)
	{
		CurrentPhaseIndex = MaxPhaseIndex;
	}

	if (WarpTargetTask)
	{
		WarpTargetTask->SetRange(GetPhaseRushDistance());
	}

	if (IsLocallyControlled())
	{
		BroadcastUIRefresh(CurrentPhaseIndex);
	}

	UpdateChargingCuePhase();

	UpdateCameraModeForPhase(CurrentPhaseIndex);
	
	StartPhaseTimer();
}

void USFGA_Thrust_HeartBreaker::OnKeyReleased(float TimeHeld)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTimerHandle);
	}
	
	// UI 숨김 메시지 전송
	BroadcastUIConstruct(false);

	// 카메라 Yaw 제한 선 해제
	DisableCameraYawLimits();

	CurrentPhaseIndex = CalculatePhase(TimeHeld);

	// 공격 Phase에 맞는 InterpSpeed 적용
	if (WarpTargetTask)
	{
		WarpTargetTask->SetInterpSpeed(GetPhaseAttackInterpSpeed());
	}

	// PhaseIndex만 서버로 전송 (MW 데이터는 CMC에서 처리)
	FScopedPredictionWindow ScopedPrediction(GetAbilitySystemComponentFromActorInfo());
	
	FSFGameplayAbilityTargetData_ChargePhase* NewData = new FSFGameplayAbilityTargetData_ChargePhase();
	NewData->PhaseIndex = CurrentPhaseIndex;
	FGameplayAbilityTargetDataHandle DataHandle(NewData);

	GetAbilitySystemComponentFromActorInfo()->ServerSetReplicatedTargetData(
		GetCurrentAbilitySpecHandle(), 
		GetCurrentActivationInfo().GetActivationPredictionKey(), 
		DataHandle, 
		FGameplayTag(), 
		GetAbilitySystemComponentFromActorInfo()->ScopedPredictionKey);
	
	// 클라이언트 예측 실행
	if (!GetAvatarActorFromActorInfo()->HasAuthority())
	{
		ExecuteRushAttack();
	}
}

void USFGA_Thrust_HeartBreaker::OnServerTargetDataReceivedCallback(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
{
	USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	}

	const FSFGameplayAbilityTargetData_ChargePhase* ReceivedData = static_cast<const FSFGameplayAbilityTargetData_ChargePhase*>(DataHandle.Get(0));
	if (!ReceivedData)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	// Phase 검증
	float ServerElapsedTime = GetWorld()->GetTimeSeconds() - AbilityStartTime;
	int32 ServerCalculatedPhase = CalculatePhase(ServerElapsedTime);

	if (ReceivedData->PhaseIndex > ServerCalculatedPhase + 1) 
	{
		CurrentPhaseIndex = ServerCalculatedPhase;
	}
	else
	{
		CurrentPhaseIndex = ReceivedData->PhaseIndex;
	}

	StopChargingCue();
	ExecuteRushAttack();
}

void USFGA_Thrust_HeartBreaker::ExecuteRushAttack()
{
	ApplySlidingMode(GetPhaseSlidingMode());
	
	UAnimMontage* RushMontage = GetPhaseRushMontage();
	
	// 돌진 몽타주 재생
	if (RushMontage)
	{
		if (UAbilityTask_PlayMontageAndWait* RushMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			TEXT("RushAttackMontage"),
			RushMontage,
			1.f,
			NAME_None,
			true))
		{
			RushMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnRushMontageFinished);
			RushMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnRushMontageFinished);
			RushMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnRushMontageFinished);
			RushMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnRushMontageFinished);
			RushMontageTask->ReadyForActivation();
		}
	}

	ClearCameraMode();
}

int32 USFGA_Thrust_HeartBreaker::CalculatePhase(float TimeHeld) const
{
	float CumulativeTime = 0.f;
	for (int32 i = 0; i < PhaseInfos.Num() - 1; ++i)
	{
		CumulativeTime += PhaseInfos[i].ChargeTimeToNext;
		if (TimeHeld < CumulativeTime)
		{
			return i; 
		}
	}
	return MaxPhaseIndex;
}

void USFGA_Thrust_HeartBreaker::OnTrace(FGameplayEventData Payload)
{
	USFAbilitySystemComponent* SourceASC = GetSFAbilitySystemComponentFromActorInfo();
	if (SourceASC == nullptr)
	{
		return;
	}

	if (!Payload.Instigator)
	{
		return;
	}

	ASFEquipmentBase* WeaponActor = const_cast<ASFEquipmentBase*>(Cast<ASFEquipmentBase>(Payload.Instigator));
	if (!WeaponActor)
	{
		return;
	}
    
	if (SourceASC->FindAbilitySpecFromHandle(CurrentSpecHandle))
	{
		FGameplayAbilityTargetDataHandle LocalTargetDataHandle(MoveTemp(Payload.TargetData));

		TArray<int32> ActorHitIndexes;
		ParseTargetData(LocalTargetDataHandle, ActorHitIndexes);

		// 차징 Phase별 데미지 사용
		const float PhaseDamage = GetPhaseDamage();

		for (int32 ActorHitIndex : ActorHitIndexes)
		{
			FHitResult HitResult = *LocalTargetDataHandle.Data[ActorHitIndex]->GetHitResult();
			ProcessHitResult(HitResult, PhaseDamage, WeaponActor);
		}
	}
}

void USFGA_Thrust_HeartBreaker::OnRushMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Thrust_HeartBreaker::ResetCharge()
{
	CurrentPhaseIndex = 0;
	TotalChargeTime = 0.f;
}

void USFGA_Thrust_HeartBreaker::BroadcastUIConstruct(bool bShow)
{
	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);

		FSFSkillProgressInfoMessage ProgressInfoMessage;
		ProgressInfoMessage.bShouldShow = bShow;
		ProgressInfoMessage.DisplayName = Name;
		ProgressInfoMessage.PhaseColor = GetPhaseColor();
		ProgressInfoMessage.TotalSkillTime = TotalChargeTime;
		MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Skill_ProgressInfoChanged, ProgressInfoMessage);
	}
}

void USFGA_Thrust_HeartBreaker::BroadcastUIRefresh(int32 NewPhaseIndex)
{
	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		FSFSkillProgressRefreshMessage Message;
		if (PhaseInfos.IsValidIndex(NewPhaseIndex))
		{
			Message.PhaseColor = PhaseInfos[NewPhaseIndex].PhaseColor;
		}
		else
		{
			Message.PhaseColor = FLinearColor::White;
		}
		MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Skill_ProgressRefresh, Message);
	}
}

void USFGA_Thrust_HeartBreaker::StartChargingCue()
{
	if (!ChargingCueTag.IsValid())
	{
		return;
	}

	USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	AActor* WeaponActor = GetMainHandWeaponActor();

	FGameplayCueParameters CueParams;
	CueParams.RawMagnitude = static_cast<float>(CurrentPhaseIndex);
	CueParams.EffectCauser = WeaponActor;
	
	// Looping Cue 시작 (OnActive -> WhileActive 호출)
	ASC->AddGameplayCue(ChargingCueTag, CueParams);
}

void USFGA_Thrust_HeartBreaker::UpdateChargingCuePhase()
{
	if (!ChargingCueTag.IsValid())
	{
		return;
	}

	USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	AActor* WeaponActor = GetMainHandWeaponActor();

	FGameplayCueParameters CueParams;
	CueParams.RawMagnitude = static_cast<float>(CurrentPhaseIndex);
	CueParams.EffectCauser = WeaponActor;
	
	// Looping Cue 갱신 (OnRecurring 호출)
	ASC->ExecuteGameplayCue(ChargingCueTag, CueParams);
}

void USFGA_Thrust_HeartBreaker::StopChargingCue()
{
	if (!ChargingCueTag.IsValid())
	{
		return;
	}

	USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// Looping Cue 종료 (OnRemove 호출)
	ASC->RemoveGameplayCue(ChargingCueTag);
}

void USFGA_Thrust_HeartBreaker::UpdateCameraModeForPhase(int32 PhaseIndex)
{
	TSubclassOf<USFCameraMode> CameraMode = nullptr;
	if (PhaseInfos.IsValidIndex(PhaseIndex))
	{
		CameraMode = PhaseInfos[PhaseIndex].CameraMode;
	}
	
	if (CameraMode)
	{
		SetCameraMode(CameraMode);
	}
}

float USFGA_Thrust_HeartBreaker::GetPhaseDamage() const
{
	if (PhaseInfos.IsValidIndex(CurrentPhaseIndex))
	{
		return GetScaledBaseDamage() * PhaseInfos[CurrentPhaseIndex].DamageMultiplier;
	}
	return GetScaledBaseDamage();
}

float USFGA_Thrust_HeartBreaker::GetPhaseRushDistance() const
{
	if (PhaseInfos.IsValidIndex(CurrentPhaseIndex))
	{
		return BaseRushDistance * PhaseInfos[CurrentPhaseIndex].RushDistanceScale;
	}
	return BaseRushDistance;
}

float USFGA_Thrust_HeartBreaker::GetPhaseAttackInterpSpeed() const
{
	if (PhaseInfos.IsValidIndex(CurrentPhaseIndex))
	{
		return PhaseInfos[CurrentPhaseIndex].AttackInterpSpeed;
	}
	return 15.f;
}

ESFSlidingMode USFGA_Thrust_HeartBreaker::GetPhaseSlidingMode() const
{
	if (PhaseInfos.IsValidIndex(CurrentPhaseIndex))
	{
		return PhaseInfos[CurrentPhaseIndex].SlidingMode;
	}
	return ESFSlidingMode::Normal;
}

UAnimMontage* USFGA_Thrust_HeartBreaker::GetPhaseRushMontage() const
{
	if (PhaseInfos.IsValidIndex(CurrentPhaseIndex))
	{
		return PhaseInfos[CurrentPhaseIndex].RushMontage;
	}
	return nullptr;
}

TSubclassOf<USFCameraMode> USFGA_Thrust_HeartBreaker::GetPhaseCameraMode() const
{
	if (PhaseInfos.IsValidIndex(CurrentPhaseIndex))
	{
		return PhaseInfos[CurrentPhaseIndex].CameraMode;
	}
	return nullptr;
}

FLinearColor USFGA_Thrust_HeartBreaker::GetPhaseColor() const
{
	if (PhaseInfos.IsValidIndex(CurrentPhaseIndex))
	{
		return PhaseInfos[CurrentPhaseIndex].PhaseColor;
	}
	return FLinearColor::White;
}

void USFGA_Thrust_HeartBreaker::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	RestoreSlidingMode();
	
	ClearCameraMode();
	
	// 타이머 정리
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTimerHandle);
	}

	// UI 숨김
	if (ActorInfo->IsLocallyControlled())
	{
		BroadcastUIConstruct(false);
	}

	StopChargingCue();

	// 슈퍼아머 제거
	if (SuperArmorEffectHandle.IsValid())
	{
		if (USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveActiveGameplayEffect(SuperArmorEffectHandle);
		}
		SuperArmorEffectHandle.Invalidate();
	}

	// 타겟 데이터 델리게이트 해제
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

	if (ASFPlayerController* SFPlayerController = GetSFPlayerControllerFromActorInfo())
	{
		SFPlayerController->SetIgnoreMoveInput(false);
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

