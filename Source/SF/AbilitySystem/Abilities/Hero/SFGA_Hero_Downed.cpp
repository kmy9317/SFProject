#include "SFGA_Hero_Downed.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/Attributes/Hero/SFCombatSet_Hero.h"
#include "AbilitySystem/Attributes/Hero/SFPrimarySet_Hero.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Animation/SFAnimationGameplayTags.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/Hero/SFHero.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "Libraries/SFAbilitySystemLibrary.h"
#include "Player/SFPlayerController.h"
#include "Player/Components/SFPlayerCombatStateComponent.h"

USFGA_Hero_Downed::USFGA_Hero_Downed(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = ESFAbilityActivationPolicy::Manual;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	AbilityTags.AddTag(SFGameplayTags::Ability_Hero_Downed);
	
	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Downed);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Downed);

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = SFGameplayTags::GameplayEvent_Downed;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void USFGA_Hero_Downed::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CachedDownedHero = Cast<ASFHero>(GetAvatarActorFromActorInfo());
	if (!CachedDownedHero.IsValid())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo())
	{
		ASC->CancelActiveAbilities(nullptr, nullptr, this);
	}

	if (USFEquipmentComponent* EquipmentComp = GetEquipmentComponent())
	{
		EquipmentComp->HideWeapons();
	}
	
	DisablePlayerInput();
	PlayDownedMontage();
	
	if (!HasAuthority(&ActivationInfo))
	{
		return;
	}

	CachedCombatStateComponent = USFPlayerCombatStateComponent::FindPlayerCombatStateComponent(GetAvatarActorFromActorInfo());
	if (!CachedCombatStateComponent.IsValid())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	float InitialGauge = CachedCombatStateComponent->GetInitialReviveGauge();
	
	// 즉시 사망 체크
	if (InitialGauge <= 0.f)
	{
		HandleDeath();
		return;
	}

	// DownCount 감소
	CachedCombatStateComponent->DecrementDownCount();

	// ReviveGauge 초기값 설정
	SetReviveGauge(InitialGauge);

	// 게이지 틱 타이머 시작
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			GaugeTickTimerHandle,
			this,
			&ThisClass::UpdateReviveGauge,
			UpdateInterval,
			true  // 반복
		);
	}
}

void USFGA_Hero_Downed::UpdateReviveGauge()
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	if (!CachedDownedHero.IsValid())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	
	float CurrentGauge = ASC->GetNumericAttribute(USFCombatSet_Hero::GetReviveGaugeAttribute());
	int32 ReviverCount = CachedDownedHero->GetActiveInteractorCount();

	// 게이지 변화량 계산 공식(TODO : 추후 상호작용 관련 버프 존재시 해당 로직 수정)
	float FillAmount = FillRatePerReviver * ReviverCount;
	float DrainAmount = DrainRatePerSecond;
	float DeltaGauge = (FillAmount - DrainAmount) * UpdateInterval;
	float NewGauge = FMath::Clamp(CurrentGauge + DeltaGauge, 0.f, MaxReviveGauge);

	SetReviveGauge(NewGauge);

	// 사망/부활 체크
	if (NewGauge <= 0.f)
	{
		HandleDeath();
	}
	else if (NewGauge >= MaxReviveGauge)
	{
		HandleRevive();
	}
}

void USFGA_Hero_Downed::SetReviveGauge(float NewValue)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->SetNumericAttributeBase(USFCombatSet_Hero::GetReviveGaugeAttribute(), NewValue);
	}
}

void USFGA_Hero_Downed::HandleDeath()
{
	// 타이머 정리
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GaugeTickTimerHandle);
	}

	SetReviveGauge(0.f);

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		USFAbilitySystemLibrary::SendDeathEvent(ASC);
	}
}

void USFGA_Hero_Downed::HandleRevive()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GaugeTickTimerHandle);
	}

	// Health 회복
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		float MaxHealth = ASC->GetNumericAttribute(USFPrimarySet_Hero::GetMaxHealthAttribute());
		float ReviveHealth = MaxHealth * ReviveHealthPercent;
		ASC->SetNumericAttributeBase(USFPrimarySet_Hero::GetHealthAttribute(), ReviveHealth);
	}

	// ReviveGauge 리셋
	SetReviveGauge(0.f);

	// 부활자들에게 이벤트 발송
	if (CachedDownedHero.IsValid())
	{
		FGameplayEventData Payload;
		Payload.EventTag = SFGameplayTags::GameplayEvent_Revived;
		Payload.Instigator = CachedDownedHero.Get();

		TArray<TWeakObjectPtr<AActor>> ReviversCopy = CachedDownedHero->GetCachedRevivers();
		for (const TWeakObjectPtr<AActor>& Reviver : ReviversCopy)
		{
			if (Reviver.IsValid())
			{
				if (UAbilitySystemComponent* ReviverASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Reviver.Get()))
				{
					ReviverASC->HandleGameplayEvent(SFGameplayTags::GameplayEvent_Revived, &Payload);
				}
			}
		}
	}
	
	// TODO: 부활 애니메이션(단발성 GameplayCue 몽타주, 이동 복구 등 추가 처리)

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Hero_Downed::DisablePlayerInput()
{
	if (ASFHero* Hero = CachedDownedHero.Get())
	{
		if (UCharacterMovementComponent* MovementComp = Hero->GetCharacterMovement())
		{
			MovementComp->StopMovementImmediately();
		}
	}

	if (ASFPlayerController* PC = GetSFPlayerControllerFromActorInfo())
	{
		PC->SetIgnoreMoveInput(true);
        
		// 카메라 회전도 막고 싶다면
		// PC->SetIgnoreLookInput(true);
	}

	// ASC 입력 버퍼 클리어
	if (IsLocallyControlled())
	{
		if (USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo())
		{
			ASC->ClearAbilityInput();
		}
	}
}

void USFGA_Hero_Downed::RestorePlayerInput()
{
	if (ASFPlayerController* PC = GetSFPlayerControllerFromActorInfo())
	{
		PC->SetIgnoreMoveInput(false);
        
		// 카메라 회전도 막았다면:
		// PC->SetIgnoreLookInput(false);
	}
}

void USFGA_Hero_Downed::PlayDownedMontage()
{
	ASFHero* Hero = Cast<ASFHero>(GetAvatarActorFromActorInfo());
	if (!Hero)
	{
		return;
	}

	if (const USFHeroAnimationData* AnimData = GetHeroAnimationData())
	{
		FSFMontagePlayData MontageData = AnimData->GetSingleMontage(SFGameplayTags::Montage_State_Downed);
		if (MontageData.IsValid() && MontageData.Montage)
		{
			if (UAbilityTask_PlayMontageAndWait* DownedMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("DownedMontage"), MontageData.Montage, MontageData.PlayRate, MontageData.StartSection, true, 1.f, 0.f, false))
			{
				DownedMontageTask->ReadyForActivation();
			}
		}
	}
}

void USFGA_Hero_Downed::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GaugeTickTimerHandle);
	}

	SetReviveGauge(0.f);

	// Death에 의한 캔슬이 아니면 복원
	bool bShouldRestore = !bWasCancelled || (CachedCombatStateComponent.IsValid() && !CachedCombatStateComponent->IsDead());

	if (bShouldRestore)
	{
		if (USFEquipmentComponent* EquipmentComp = GetEquipmentComponent())
		{
			EquipmentComp->ShowWeapons();
		}

		FSFMontagePlayData MontageData = GetMainHandEquipMontageData();
		ExecuteMontageGameplayCue(MontageData);
		
	}
	RestorePlayerInput();
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
