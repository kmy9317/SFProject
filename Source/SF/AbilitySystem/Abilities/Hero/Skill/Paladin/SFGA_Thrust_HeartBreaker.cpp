// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGA_Thrust_HeartBreaker.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/SFCharacterBase.h"
#include "MotionWarpingComponent.h"
#include "AbilitySystem/GameplayEffect/Hero/EffectContext/SFTargetDataTypes.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Messages/SFMessageGameplayTags.h"
#include "Messages/SFSkillInfoMessages.h"
#include "Player/SFPlayerController.h"
#include "Weapons/Actor/SFEquipmentBase.h"

USFGA_Thrust_HeartBreaker::USFGA_Thrust_HeartBreaker(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PhaseTimes.SetNum(2);
	PhaseColors.SetNum(3);
}

void USFGA_Thrust_HeartBreaker::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AbilityStartTime = GetWorld()->GetTimeSeconds();
	MaxPhaseIndex = PhaseTimes.Num(); 
	
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
		
		for (float PhaseTime : PhaseTimes)
		{
			TotalChargeTime += PhaseTime;
		}
		
		// UI 표시
		BroadcastUIConstruct(true);
	}

	StartChargingCue();
	StartPhaseTimer();

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
	
	if (PhaseTimes.IsValidIndex(CurrentPhaseIndex))
	{
		float PhaseTime = PhaseTimes[CurrentPhaseIndex];
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

	if (IsLocallyControlled())
	{
		BroadcastUIRefresh(CurrentPhaseIndex);
	}

	UpdateChargingCuePhase();
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

	// TimeHeld를 기준으로 페이즈 재확정 (UI 타이머와 미세한 오차가 있을 수 있으므로)
	CurrentPhaseIndex = CalculatePhase(TimeHeld);

	// Motion Warping 타겟 위치/회전 계산
	FVector TargetLocation = CalculateRushTargetLocation();
	FRotator TargetRotation = CalculateRushTargetRotation();

	// 서버로 보낼 TargetData 패키징 
	FScopedPredictionWindow ScopedPrediction(GetAbilitySystemComponentFromActorInfo());
	
	FSFGameplayAbilityTargetData_ChargePhase* NewData = new FSFGameplayAbilityTargetData_ChargePhase();
	NewData->PhaseIndex = CurrentPhaseIndex;
	NewData->RushTargetLocation = TargetLocation;
	NewData->RushTargetRotation = TargetRotation;
	FGameplayAbilityTargetDataHandle DataHandle(NewData);

	// 서버로 전송
	GetAbilitySystemComponentFromActorInfo()->ServerSetReplicatedTargetData(
			GetCurrentAbilitySpecHandle(), 
			GetCurrentActivationInfo().GetActivationPredictionKey(), 
			DataHandle, 
			FGameplayTag(), 
			GetAbilitySystemComponentFromActorInfo()->ScopedPredictionKey);
	
	// 클라이언트인 경우 반응성을 위해 즉시 예측 실행
	if (!GetAvatarActorFromActorInfo()->HasAuthority())
	{
		SetupMotionWarpingTarget(TargetLocation);

		if (ASFCharacterBase* Character = GetSFCharacterFromActorInfo())
		{
			Character->SetActorRotation(TargetRotation);
		}
		
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

	float ServerElapsedTime = GetWorld()->GetTimeSeconds() - AbilityStartTime;
	int32 ServerCalculatedPhase = CalculatePhase(ServerElapsedTime);

	// 오차 검증 (네트워크 RTT 고려하여 클라이언트 페이즈가 서버보다 1단계 정도 까지만 허용)
	if (ReceivedData->PhaseIndex > ServerCalculatedPhase + 1) 
	{
		// 오차를 벗어난 경우
		CurrentPhaseIndex = ServerCalculatedPhase;
	}
	else
	{
		// 오차를 허용한 경우
		CurrentPhaseIndex = ReceivedData->PhaseIndex;
	}

	StopChargingCue();
	SetupMotionWarpingTarget(ReceivedData->RushTargetLocation);

	if (ASFCharacterBase* Character = GetSFCharacterFromActorInfo())
	{
		Character->SetActorRotation(ReceivedData->RushTargetRotation);
	}
	
	ExecuteRushAttack();
}

void USFGA_Thrust_HeartBreaker::SetupMotionWarpingTarget(const FVector& TargetLocation)
{
	if (ASFCharacterBase* SFCharacter = GetSFCharacterFromActorInfo())
    {
        if (UMotionWarpingComponent* MW = SFCharacter->GetMotionWarpingComponent())
        {
        	MW->AddOrUpdateWarpTargetFromLocation(WarpTargetName, TargetLocation);
        }
    }
}

void USFGA_Thrust_HeartBreaker::ExecuteRushAttack()
{
	UAnimMontage* RushAttackMontage = nullptr;

	if (RushAttackMontages.IsValidIndex(CurrentPhaseIndex))
	{
		RushAttackMontage = RushAttackMontages[CurrentPhaseIndex];
	}
	
	// 돌진 몽타주 재생
	if (RushAttackMontage)
	{
		if (UAbilityTask_PlayMontageAndWait* RushMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			TEXT("RushAttackMontage"),
			RushAttackMontage,
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
}

int32 USFGA_Thrust_HeartBreaker::CalculatePhase(float TimeHeld) const
{
	float CumulativeTime = 0.f;
	for (int32 i = 0; i < PhaseTimes.Num(); ++i)
	{
		CumulativeTime += PhaseTimes[i];
		if (TimeHeld < CumulativeTime)
		{
			return i; 
		}
	}
	return MaxPhaseIndex;
}

FVector USFGA_Thrust_HeartBreaker::CalculateRushTargetLocation() const
{
	ASFCharacterBase* SFCharacter = GetSFCharacterFromActorInfo();
	if (!SFCharacter)
	{
		return FVector::ZeroVector;
	}

	float Dist = GetPhaseRushDistance();
    
	// 로컬 플레이어의 마지막 입력 의도 방향 가져오기
	FVector AttackDirection = SFCharacter->GetLastInputDirection();

	// 입력이 없다면 캐릭터의 전방 사용
	if (AttackDirection.IsNearlyZero())
	{
		AttackDirection = SFCharacter->GetActorForwardVector();
	}

	return SFCharacter->GetActorLocation() + (AttackDirection * Dist);
}

FRotator USFGA_Thrust_HeartBreaker::CalculateRushTargetRotation() const
{
	ASFCharacterBase* SFCharacter = GetSFCharacterFromActorInfo();
	if (!SFCharacter)
	{
		return FRotator::ZeroRotator;
	}

	FVector AttackDirection = SFCharacter->GetLastInputDirection();
	
	if (AttackDirection.IsNearlyZero())
	{
		AttackDirection = SFCharacter->GetActorForwardVector();
	}

	return AttackDirection.Rotation();
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
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);

	FSFSkillProgressInfoMessage ProgressInfoMessage;
	ProgressInfoMessage.bShouldShow = bShow;
	ProgressInfoMessage.DisplayName = Name;
	ProgressInfoMessage.PhaseColor = PhaseColors[CurrentPhaseIndex];
	ProgressInfoMessage.TotalSkillTime = TotalChargeTime;
	MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Skill_ProgressInfoChanged, ProgressInfoMessage);
}

void USFGA_Thrust_HeartBreaker::BroadcastUIRefresh(int32 NewPhaseIndex)
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);

	if (CurrentPhaseIndex < PhaseTimes.Num())
	{
		FSFSkillProgressRefreshMessage Message;
		Message.PhaseColor = PhaseColors[NewPhaseIndex];
		MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Skill_ProgressRefresh, Message);
	}
	else
	{
		FSFSkillProgressRefreshMessage Message;
		Message.PhaseColor = PhaseColors[MaxPhaseIndex];
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

float USFGA_Thrust_HeartBreaker::GetPhaseDamage() const
{
	if (PhaseDamageMultipliers.IsValidIndex(CurrentPhaseIndex))
	{
		return BaseDamage * PhaseDamageMultipliers[CurrentPhaseIndex];
	}
	return BaseDamage;
}

float USFGA_Thrust_HeartBreaker::GetPhaseRushDistance() const
{
	if (PhaseRushDistanceScales.IsValidIndex(CurrentPhaseIndex))
	{
		return BaseRushDistance * PhaseRushDistanceScales[CurrentPhaseIndex];
	}
	return BaseRushDistance;
}

void USFGA_Thrust_HeartBreaker::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
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

	// Motion Warping 타겟 제거
	if (ASFCharacterBase* SFCharacter = GetSFCharacterFromActorInfo())
	{
		if (UMotionWarpingComponent* MotionWarpingComp = SFCharacter->FindComponentByClass<UMotionWarpingComponent>())
		{
			MotionWarpingComp->RemoveWarpTarget(WarpTargetName);
		}
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

