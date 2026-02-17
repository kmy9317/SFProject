// SFGA_Projectile_Charged.cpp

#include "SFGA_Projectile_Charged.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Messages/SFMessageGameplayTags.h"
#include "Messages/SFSkillInfoMessages.h"
#include "AbilitySystem/GameplayEffect/Hero/EffectContext/SFTargetDataTypes.h" // TargetData 구조체 위치 가정
#include "Player/SFPlayerController.h"
#include "Actors/SFAttackProjectile.h"
#include "Character/SFCharacterBase.h"

USFGA_Projectile_Charged::USFGA_Projectile_Charged(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USFGA_Projectile_Charged::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    AbilityStartTime = GetWorld()->GetTimeSeconds();
    MaxPhaseIndex = PhaseInfos.Num() > 0 ? PhaseInfos.Num() - 1 : 0;
    
    ResetCharge();

    // 1. 차징 몽타주 재생 (Loop)
    if (ChargingMontage)
    {
        UAbilityTask_PlayMontageAndWait* ChargeTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, TEXT("Charging"), ChargingMontage, 1.f, NAME_None, false
        );
        if (ChargeTask)
        {
            ChargeTask->ReadyForActivation();
        }
    }

    // 2. 이동 제어 (선택 사항)
	if (ASFPlayerController* PC = GetSFPlayerControllerFromActorInfo())
	{
		PC->SetIgnoreMoveInput(true);
	}

    // 3. 로컬 클라이언트: 입력 대기 및 UI
    if (ActorInfo->IsLocallyControlled())
    {
        UAbilityTask_WaitInputRelease* WaitTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
        if (WaitTask)
        {
            WaitTask->OnRelease.AddDynamic(this, &ThisClass::OnKeyReleased);
            WaitTask->ReadyForActivation();
        }

        TotalChargeTime = 0.f;
		for (int32 i = 0; i < PhaseInfos.Num() - 1; ++i)
		{
			TotalChargeTime += PhaseInfos[i].ChargeTimeToNext;
		}
        BroadcastUIConstruct(true);
    }
    
    // 4. 타이머 및 큐 시작
    StartPhaseTimer();
    
    // GameplayCue 시작 (차징 이펙트/사운드)
    if (ChargingCueTag.IsValid())
    {
        FGameplayCueParameters CueParams;
        CueParams.RawMagnitude = static_cast<float>(CurrentPhaseIndex);
        CueParams.EffectCauser = GetAvatarActorFromActorInfo();
        GetSFAbilitySystemComponentFromActorInfo()->AddGameplayCue(ChargingCueTag, CueParams);
    }

    // 5. 서버: 클라이언트 TargetData 대기
    if (ActorInfo->IsNetAuthority())
	{
		USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			FAbilityTargetDataSetDelegate& TargetDataDelegate = ASC->AbilityTargetDataSetDelegate(
				CurrentSpecHandle, 
				CurrentActivationInfo.GetActivationPredictionKey()
			);
			ServerTargetDataDelegateHandle = TargetDataDelegate.AddUObject(this, &ThisClass::OnServerTargetDataReceivedCallback);
			ASC->CallReplicatedTargetDataDelegatesIfSet(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
		}
	}
}

void USFGA_Projectile_Charged::StartPhaseTimer()
{
    if (CurrentPhaseIndex >= MaxPhaseIndex) return;

    if (PhaseInfos.IsValidIndex(CurrentPhaseIndex))
    {
        GetWorld()->GetTimerManager().SetTimer(
            PhaseTimerHandle,
            this,
            &ThisClass::OnPhaseTimePassed,
            PhaseInfos[CurrentPhaseIndex].ChargeTimeToNext,
            false
        );
    }
}

void USFGA_Projectile_Charged::OnPhaseTimePassed()
{
    CurrentPhaseIndex++;
    if (CurrentPhaseIndex > MaxPhaseIndex) CurrentPhaseIndex = MaxPhaseIndex;

    if (IsLocallyControlled())
    {
        BroadcastUIRefresh(CurrentPhaseIndex);
    }
    
    // Cue 업데이트 (단계 변화 표현)
    if (ChargingCueTag.IsValid())
    {
        FGameplayCueParameters CueParams;
        CueParams.RawMagnitude = static_cast<float>(CurrentPhaseIndex);
        CueParams.EffectCauser = GetAvatarActorFromActorInfo();
        GetSFAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(ChargingCueTag, CueParams);
    }

    StartPhaseTimer();
}

void USFGA_Projectile_Charged::OnKeyReleased(float TimeHeld)
{
    GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
    BroadcastUIConstruct(false);

    CurrentPhaseIndex = CalculatePhase(TimeHeld);

    // 서버로 결정된 Phase 전송
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

    if (!HasAuthority(&CurrentActivationInfo))
    {
        PlayLaunchMontage();
    }
}

void USFGA_Projectile_Charged::OnServerTargetDataReceivedCallback(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
{
    USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	}

    const FSFGameplayAbilityTargetData_ChargePhase* ReceivedData = static_cast<const FSFGameplayAbilityTargetData_ChargePhase*>(DataHandle.Get(0));
    if (ReceivedData)
    {
        // 서버 검증 (간략화)
        float ServerElapsedTime = GetWorld()->GetTimeSeconds() - AbilityStartTime;
        int32 ServerPhase = CalculatePhase(ServerElapsedTime);
        
        // 너무 앞서간 Phase가 아니면 클라이언트 값 수용, 아니면 서버 계산값 사용
        if (ReceivedData->PhaseIndex > ServerPhase + 1)
             CurrentPhaseIndex = ServerPhase;
        else
             CurrentPhaseIndex = ReceivedData->PhaseIndex;
    }

    PlayLaunchMontage();
}

void USFGA_Projectile_Charged::PlayLaunchMontage()
{
    // 차징 큐 제거
    if (ChargingCueTag.IsValid())
    {
        GetSFAbilitySystemComponentFromActorInfo()->RemoveGameplayCue(ChargingCueTag);
    }

    // 부모 로직의 발사 시퀀스 시작 (WaitEventTask + MontagePlay)
    
    // 1. 발사 이벤트 대기 (부모 로직과 동일)
    // 중요: 부모의 ActivateAbility를 안 썼으므로 WaitEventTask를 여기서 수동 설정해야 함
    WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this,ProjectileSpawnEventTag,nullptr,true,true);
    if (WaitEventTask)
	{
		WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnProjectileSpawnEventReceived);
		WaitEventTask->ReadyForActivation();
	}

    // 2. 발사 몽타주 재생
    // LaunchMontage는 부모 클래스의 변수 사용
    if (LaunchMontage)
    {
        MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this,NAME_None,LaunchMontage,LaunchMontagePlayRate);
        if (MontageTask)
        {
            MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
            MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
            MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
        	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
            MontageTask->ReadyForActivation();
        }
    }
    else
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
    }
}

void USFGA_Projectile_Charged::OnProjectileSpawnEventReceived(FGameplayEventData Payload)
{
    // 부모의 로직을 복사하되, SpawnProjectile 호출 시 파라미터를 변경하거나
    // SpawnProjectile_Server를 호출하기 전에 Phase 데이터를 적용해야 함.
    // 하지만 SpawnProjectile_Server는 파라미터가 고정되어 있으므로, 직접 구현합니다.

    if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		K2_CancelAbility();
		return;
	}

    // 스폰 위치 계산 (부모 함수 재사용)
    FTransform SpawnTM;
    if (!GetProjectileSpawnTransform(SpawnTM))
    {
       // Fallback logic from parent
       if (ASFCharacterBase* Character = GetSFCharacterFromActorInfo())
	   {
			SpawnTM = FTransform(Character->GetActorRotation(), Character->GetActorLocation() + FallbackSpawnOffset);
	   }
    }

    const FVector LaunchDir = GetLaunchDirection();

    if (HasAuthority(&CurrentActivationInfo))
    {
        // === [핵심] 차징된 데이터 적용 ===
        float FinalDamage = GetScaledBaseDamage();
        float FinalScale = 1.0f;
        bool bFinalExplode = false;

        if (PhaseInfos.IsValidIndex(CurrentPhaseIndex))
        {
            const auto& Phase = PhaseInfos[CurrentPhaseIndex];
            FinalDamage *= Phase.DamageMultiplier;
            FinalScale = Phase.ProjectileScale;
            bFinalExplode = Phase.bEnableExplosion;
        }

        // 직접 스폰 로직 구현 (부모의 SpawnProjectile_Server 내용을 수정하여 적용)
        UWorld* World = GetWorld();
	    ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	    USFAbilitySystemComponent* SourceASC = GetSFAbilitySystemComponentFromActorInfo();
        
        if (World && Character && SourceASC && ProjectileClass)
        {
            FActorSpawnParameters Params;
            Params.Owner = Character;
            Params.Instigator = Cast<APawn>(Character);
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            ASFAttackProjectile* Projectile = World->SpawnActor<ASFAttackProjectile>(
                ProjectileClass,
                SpawnTM.GetLocation(),
                LaunchDir.Rotation(),
                Params
            );

            if (Projectile)
            {
                // [수정] InitProjectileCharged 호출
                Projectile->InitProjectileCharged(SourceASC, FinalDamage, Character, FinalScale, bFinalExplode);
                Projectile->Launch(LaunchDir);
            }
        }
    }
    
    // Montage 종료 대기 (EndAbility 호출 안함)
}

int32 USFGA_Projectile_Charged::CalculatePhase(float TimeHeld) const
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

void USFGA_Projectile_Charged::ResetCharge()
{
	CurrentPhaseIndex = 0;
	TotalChargeTime = 0.f;
}

// UI 및 기타 헬퍼 함수들은 HeartBreaker와 동일하게 구현
void USFGA_Projectile_Charged::BroadcastUIConstruct(bool bShow)
{
	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);

		FSFSkillProgressInfoMessage ProgressInfoMessage;
		ProgressInfoMessage.bShouldShow = bShow;
		ProgressInfoMessage.DisplayName = Name; // 부모 or GA Name
		
        if (PhaseInfos.IsValidIndex(CurrentPhaseIndex))
            ProgressInfoMessage.PhaseColor = PhaseInfos[CurrentPhaseIndex].PhaseColor;
		else
            ProgressInfoMessage.PhaseColor = FLinearColor::White;

		ProgressInfoMessage.TotalSkillTime = TotalChargeTime;
		MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Skill_ProgressInfoChanged, ProgressInfoMessage);
	}
}

void USFGA_Projectile_Charged::BroadcastUIRefresh(int32 NewPhaseIndex)
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

void USFGA_Projectile_Charged::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    // 타이머 해제
    GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
    
    // UI 정리
    if (ActorInfo->IsLocallyControlled())
    {
        BroadcastUIConstruct(false);
    }
    
    // 이동 제어 해제
    if (ASFPlayerController* PC = GetSFPlayerControllerFromActorInfo())
    {
        PC->SetIgnoreMoveInput(false);
    }

    // 서버 델리게이트 해제
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
    }
    
    // 부모의 EndAbility (Task 정리 등)
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}