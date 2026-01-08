#include "SFGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemLog.h"
#include "AbilityCost/SFAbilityCost.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/GameplayCues/SFGameplayCueTags.h"
#include "Camera/SFCameraMode.h"
#include "Character/SFCharacterBase.h"
#include "Character/SFPawnData.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Character/Hero/SFHeroComponent.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "Input/SFEnhancedPlayerInput.h"
#include "Player/SFPlayerController.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFHeroSkillTags.h"
#include "Player/SFPlayerState.h"

#define ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(FunctionName, ReturnValue)																				\
{																																						\
	if (!ensure(IsInstantiated()))																														\
	{																																					\
	ABILITY_LOG(Error, TEXT("%s: " #FunctionName " cannot be called on a non-instanced ability. Check the instancing policy."), *GetPathName());		\
	return ReturnValue;																																	\
	}																																					\
}


USFGameplayAbility::USFGameplayAbility(const FObjectInitializer& ObjectInitializer)
{
	ActivationPolicy = ESFAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool USFGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	// 추가 Cost 체크
	for (const USFAbilityCost* Cost : AdditionalCosts)
	{
		if (Cost && !Cost->CheckCost(this, Handle, ActorInfo, OptionalRelevantTags))
		{
			return false;
		}
	}

	return true;
}

void USFGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

	// 추가 Cost 적용
	for (USFAbilityCost* Cost : AdditionalCosts)
	{
		if (Cost && !Cost->ShouldOnlyApplyCostOnHit())
		{
			Cost->ApplyCost(this, Handle, ActorInfo, ActivationInfo);
		}
	}
}

USFAbilitySystemComponent* USFGameplayAbility::GetSFAbilitySystemComponentFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<USFAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr);
}

ASFPlayerController* USFGameplayAbility::GetSFPlayerControllerFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<ASFPlayerController>(CurrentActorInfo->PlayerController.Get()) : nullptr);
}

ASFPlayerState* USFGameplayAbility::GetSFPlayerStateFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<ASFPlayerState>(CurrentActorInfo->OwnerActor.Get()) : nullptr);
}

ASFCharacterBase* USFGameplayAbility::GetSFCharacterFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<ASFCharacterBase>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

USFHeroMovementComponent* USFGameplayAbility::GetHeroMovementComponentFromActorInfo() const
{
	return (CurrentActorInfo && CurrentActorInfo->AvatarActor.Get()) 
		? CurrentActorInfo->AvatarActor->FindComponentByClass<USFHeroMovementComponent>() 
		: nullptr;
}

USFHeroComponent* USFGameplayAbility::GetHeroComponentFromActorInfo() const
{
	return (CurrentActorInfo ? USFHeroComponent::FindHeroComponent(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

USFHeroAnimationData* USFGameplayAbility::GetHeroAnimationData() const
{
	if (const USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(GetAvatarActorFromActorInfo()))
	{
		if (const USFPawnData* PawnData = PawnExtComp->GetPawnData<USFPawnData>())
		{
			return PawnData->HeroAnimationData;
		}
	}
	return nullptr;
}

USFEquipmentComponent* USFGameplayAbility::GetEquipmentComponent() const
{
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		return USFEquipmentComponent::FindEquipmentComponent(AvatarActor);
	}
	return nullptr;
}

FSFMontagePlayData USFGameplayAbility::GetMainHandEquipMontageData() const
{
	USFEquipmentComponent* EquipmentComp = GetEquipmentComponent();
	USFHeroAnimationData* AnimData = GetHeroAnimationData();

	if (EquipmentComp && AnimData)
	{
		FGameplayTag MontageTag = EquipmentComp->GetMainHandEquipMontageTag();
		if (MontageTag.IsValid())
		{
			return AnimData->GetSingleMontage(MontageTag);
		}
	}
	return FSFMontagePlayData();
}

ETeamAttitude::Type USFGameplayAbility::GetAttitudeTowards(AActor* Target) const
{
	if (!Target)
		return ETeamAttitude::Neutral;
	AActor* SourceActor = GetAvatarActorFromActorInfo();
	if (!SourceActor)
		return ETeamAttitude::Neutral;

	// Source의 TeamAgent 가져오기
	IGenericTeamAgentInterface* SourceTeamAgent = Cast<IGenericTeamAgentInterface>(SourceActor);
	if (!SourceTeamAgent)
		return ETeamAttitude::Neutral;
	// Target의 TeamAgent 가져오기
	IGenericTeamAgentInterface* TargetTeamAgent = Cast<IGenericTeamAgentInterface>(Target);
	if (!TargetTeamAgent)
		return ETeamAttitude::Neutral;
	// 적대 관계 확인
	return FGenericTeamId::GetAttitude(
	   SourceTeamAgent->GetGenericTeamId(),
	   TargetTeamAgent->GetGenericTeamId()
   );
}

void USFGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	TryActivateAbilityOnSpawn(ActorInfo, Spec);
}

void USFGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (bAutoApplyDurationEffect && DurationGameplayEffectClass)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, DurationGameplayEffectClass, 1.0f);

		if (SpecHandle.IsValid())
		{
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}
}

AController* USFGameplayAbility::GetControllerFromActorInfo() const
{
	if (CurrentActorInfo)
	{
		if (AController* PC = CurrentActorInfo->PlayerController.Get())
		{
			return PC;
		}

		// Look for a player controller or pawn in the owner chain.
		AActor* TestActor = CurrentActorInfo->OwnerActor.Get();
		while (TestActor)
		{
			if (AController* C = Cast<AController>(TestActor))
			{
				return C;
			}

			if (APawn* Pawn = Cast<APawn>(TestActor))
			{
				return Pawn->GetController();
			}

			TestActor = TestActor->GetOwner();
		}
	}

	return nullptr;
}

void USFGameplayAbility::TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const
{
	// Try to activate if activation policy is on spawn.
	if (ActorInfo && !Spec.IsActive() && (ActivationPolicy == ESFAbilityActivationPolicy::OnSpawn))
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		const AActor* AvatarActor = ActorInfo->AvatarActor.Get();

		// If avatar actor is torn off or about to die, don't try to activate until we get the new one.
		if (ASC && AvatarActor && !AvatarActor->GetTearOff() && (AvatarActor->GetLifeSpan() <= 0.0f))
		{
			const bool bIsLocalExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted) || (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalOnly);
			const bool bIsServerExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly) || (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerInitiated);

			const bool bClientShouldActivate = ActorInfo->IsLocallyControlled() && bIsLocalExecution;
			const bool bServerShouldActivate = ActorInfo->IsNetAuthority() && bIsServerExecution;

			if (bClientShouldActivate || bServerShouldActivate)
			{
				ASC->TryActivateAbility(Spec.Handle);
			}
		}
	}
}

void USFGameplayAbility::SetCameraMode(TSubclassOf<USFCameraMode> CameraMode)
{
	ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(SetCameraMode, );

	if (USFHeroComponent* HeroComponent = GetHeroComponentFromActorInfo())
	{
		HeroComponent->SetAbilityCameraMode(CameraMode, CurrentSpecHandle);
		ActiveCameraMode = CameraMode;
	}
}

void USFGameplayAbility::ClearCameraMode()
{
	ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(ClearCameraMode, );

	if (ActiveCameraMode)
	{
		if (USFHeroComponent* HeroComponent = GetHeroComponentFromActorInfo())
		{
			HeroComponent->ClearAbilityCameraMode(CurrentSpecHandle);
		}

		ActiveCameraMode = nullptr;
	}
}

void USFGameplayAbility::DisableCameraYawLimits()
{
	ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(DisableCameraYawLimits, );
	
	if (ActiveCameraMode)
	{
		if (USFHeroComponent* HeroComponent = GetHeroComponentFromActorInfo())
		{
			HeroComponent->DisableAbilityCameraYawLimits();
		}
	}
}

void USFGameplayAbility::DisableCameraYawLimitsForActiveMode()
{
	ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(DisableCameraYawLimitsForActiveMode, );
    
	if (ActiveCameraMode)
	{
		if (USFHeroComponent* HeroComponent = GetHeroComponentFromActorInfo())
		{
			HeroComponent->DisableAbilityCameraYawLimitsForMode(ActiveCameraMode);
		}
	}
}

void USFGameplayAbility::ApplySlidingMode(ESFSlidingMode NewMode)
{
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (!Character)
	{
		return;
	}

	USFHeroMovementComponent* HeroCMC = GetHeroMovementComponentFromActorInfo();
	if (!HeroCMC)
	{
		return;
	}

	// 이미 적용 중이면 원본 덮어쓰지 않음
	if (!bSlidingModeApplied)
	{
		SavedSlidingMode = HeroCMC->SlidingMode;
		bSlidingModeApplied = true;
	}

	HeroCMC->SetSlidingMode(NewMode);
}

void USFGameplayAbility::RestoreSlidingMode()
{
	if (!bSlidingModeApplied)
	{
		return;
	}

	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (!Character)
	{
		return;
	}

	USFHeroMovementComponent* CMC = Cast<USFHeroMovementComponent>(Character->GetCharacterMovement());
	if (CMC)
	{
		CMC->SetSlidingMode(SavedSlidingMode);
	}

	bSlidingModeApplied = false;
}

void USFGameplayAbility::TryProcCooldownReset()
{
	// 서버에서만
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// 패시브 없으면 패스
	if (!ASC->HasMatchingGameplayTag(SFGameplayTags::Ability_Skill_Passive_CooldownReset))
	{
		return;
	}

	// 확률 체크
	const float ProcChance = 0.25f;
	if (FMath::FRand() > ProcChance)
	{
		return;
	}

	// 이 Ability가 사용하는 쿨다운 태그 얻기
	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	if (!CooldownTags || CooldownTags->IsEmpty())
	{
		return;
	}

	// 이 Ability의 쿨다운 GE 제거
	ASC->RemoveActiveEffectsWithGrantedTags(*CooldownTags);
}

bool USFGameplayAbility::CommitAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	FGameplayTagContainer* OptionalRelevantTags
)
{
	// 기존 GAS 커밋 로직 그대로 수행
	const bool bCommitted = Super::CommitAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		OptionalRelevantTags
	);

	// 커밋 성공한 경우에만 후처리
	if (bCommitted)
	{
		TryProcCooldownReset();
	}

	return bCommitted;
}

bool USFGameplayAbility::CommitAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo
)
{
	// 모든 기존 스킬 호출은 여기로
	return CommitAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		nullptr
	);
}

void USFGameplayAbility::FlushPressedInput(UInputAction* InputAction)
{
	if (CurrentActorInfo)
	{
		if (APlayerController* PlayerController = CurrentActorInfo->PlayerController.Get())
		{
			if (USFEnhancedPlayerInput* PlayerInput = Cast<USFEnhancedPlayerInput>(PlayerController->PlayerInput))
			{
				PlayerInput->FlushPressedInput(InputAction);
			}
		}
	}
}

void USFGameplayAbility::ExecuteMontageGameplayCue(const FSFMontagePlayData& MontageData)
{
	if (!MontageData.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	FGameplayCueParameters CueParams;
	CueParams.SourceObject = MontageData.Montage;
	CueParams.RawMagnitude = MontageData.PlayRate;
	
	// StartSection은 필요시 다른 파라미터에 저장
	ASC->ExecuteGameplayCue(SFGameplayTags::GameplayCue_Animation_PlayMontage ,CueParams);
}
