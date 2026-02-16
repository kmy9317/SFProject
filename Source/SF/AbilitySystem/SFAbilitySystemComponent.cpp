// Fill out your copyright notice in the Description page of Project Settings.


#include "SFAbilitySystemComponent.h"

#include "SFLogChannels.h"
#include "Abilities/SFGameplayAbility.h"
#include "Abilities/Hero/Skill/SFHeroSkillTags.h"
#include "Animation/Enemy/SFEnemyAnimInstance.h"
#include "Animation/Hero/SFHeroAnimInstance.h"
#include "Attributes/SFPrimarySet.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/Enemy/SFEnemy.h"
#include "Character/Hero/SFHero.h"
#include "GameplayEffect/SFGameplayEffectTags.h"
#include "Player/Save/SFPersistentDataType.h"


USFAbilitySystemComponent::USFAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputStartedSpecHandles.Reset();
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();

	InputBlockedTags.AddTag(SFGameplayTags::Character_State_Downed);
	InputBlockedTags.AddTag(SFGameplayTags::Character_State_Dead);
}

void USFAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
	check(ActorInfo);
	check(InOwnerActor);

	// 새로운 Pawn Avatar가 설정되었는지 확인
	const bool bHasNewPawnAvatar = Cast<APawn>(InAvatarActor) && (InAvatarActor != ActorInfo->AvatarActor);
	
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (bHasNewPawnAvatar)
	{
		// TODO : AnimInstance 초기화
		ASFEnemy* Enemy = Cast<ASFEnemy>(GetAvatarActor());
		if (Enemy)
		{
			USFEnemyAnimInstance* AniminInstance = Cast<USFEnemyAnimInstance>(GetAvatarActor()->FindComponentByClass<USkeletalMeshComponent>()->GetAnimInstance());
			if (IsValid(AniminInstance))
			{
				AniminInstance->InitializeWithAbilitySystem(this); 
			}
		}
		else
		{
			ASFHero* Hero = Cast<ASFHero>(GetAvatarActor());
			if (Hero)
			{
				USFHeroAnimInstance* AniminInstance = Cast<USFHeroAnimInstance>(GetAvatarActor()->FindComponentByClass<USkeletalMeshComponent>()->GetAnimInstance());
				if (IsValid(AniminInstance))
				{
					AniminInstance->InitializeWithAbilitySystem(this);
				}
			}
		}
		// 스폰 시 자동으로 활성화되어야 하는 어빌리티들 처리
		TryActivateAbilitiesOnSpawn();
	}
}

void USFAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);

	if (AbilityChangedDelegate.IsBound())
	{
		AbilityChangedDelegate.Broadcast(AbilitySpec.Handle, true);
	}
}

void USFAbilitySystemComponent::OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	if (AbilityChangedDelegate.IsBound())
	{
		AbilityChangedDelegate.Broadcast(AbilitySpec.Handle, false);
	}
	
	Super::OnRemoveAbility(AbilitySpec);
}

void USFAbilitySystemComponent::TryActivateAbilitiesOnSpawn()
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (const USFGameplayAbility* SFAbilityCDO = Cast<USFGameplayAbility>(AbilitySpec.Ability))
		{
			SFAbilityCDO->TryActivateAbilityOnSpawn(AbilityActorInfo.Get(), AbilitySpec);
		}
	}
}

void USFAbilitySystemComponent::AbilitySpecInputStarted(FGameplayAbilitySpec& Spec)
{
	if (Spec.IsActive())
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		FPredictionKey OriginalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::GameCustom1, Spec.Handle, OriginalPredictionKey);
	}
}

void USFAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	if (Spec.IsActive())
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		FPredictionKey OriginalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, OriginalPredictionKey);
	}
}

void USFAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	if (Spec.IsActive())
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		FPredictionKey OriginalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, OriginalPredictionKey);
	}
}

void USFAbilitySystemComponent::AbilityInputTagStarted(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			{
				InputStartedSpecHandles.AddUnique(AbilitySpec.Handle);
			}
		}
	}
}

void USFAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			{
				// 입력을 큐에 넣어서 ProcessAbilityInput에서 처리하게 함
				InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);

				// 콤보 시스템을 위한 핵심 로직
				// 만약 이 어빌리티가 이미 '활성화(Active)' 상태라면? -> 콤보 입력으로 간주
				if (AbilitySpec.IsActive())
				{
					// "Input.Action.Attack" 같은 태그를 Payload에 담음
					FGameplayEventData Payload;
					Payload.EventTag = InputTag;
					Payload.Instigator = GetAvatarActor();
					Payload.Target = GetAvatarActor();

					// 이벤트를 즉시 발송 -> GA 블루프린트의 'Wait Gameplay Event'가 이걸 받음
					HandleGameplayEvent(InputTag, &Payload);
				}
			}
		}
	}
}

void USFAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			{
				InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.Remove(AbilitySpec.Handle);
			}
		}
	}
}

void USFAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	// 입력 차단 태그 체크 - 있으면 입력 버퍼만 클리어하고 리턴
	if (HasAnyMatchingGameplayTags(InputBlockedTags))
	{
		InputStartedSpecHandles.Reset();
		InputPressedSpecHandles.Reset();
		InputReleasedSpecHandles.Reset();
		// InputHeldSpecHandles는 유지 (키를 떼면 정상적으로 제거되도록)
		return;
	}

	// 이번 프레임에 활성화할 어빌리티들을 저장할 정적 배열
	// 정적 변수로 선언하여 매 프레임마다 메모리 할당을 피함
	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

	//
	// 1단계: 지속 입력으로 활성화되는 어빌리티들 처리 (WhileInputActive 정책)
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability && !AbilitySpec->IsActive())
			{
				const USFGameplayAbility* SFAbilityCDO = Cast<USFGameplayAbility>(AbilitySpec->Ability);
				// 지속 입력으로 활성화되는 어빌리티들 처리 (WhileInputActive 정책)
				if (SFAbilityCDO && SFAbilityCDO->GetActivationPolicy() == ESFAbilityActivationPolicy::WhileInputActive)
				{
					AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
				}
			}
		}
	}

	//
	// 2단계: 입력 시작 이벤트 처리 (Started 이벤트 → GameCustom1 이벤트 발생)
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputStartedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				// 활성화된 어빌리티만 Started 이벤트를 전달
				if (AbilitySpec->IsActive())
				{
					// GameCustom1 복제 이벤트 발생 (USFAbilityTask_WaitInputStart가 감지)
					AbilitySpecInputStarted(*AbilitySpec);
				}
			}
		}
	}

	// 입력 pressed 이벤트 처리 (Pressed 이벤트 → 활성화 vs 입력 이벤트 분기)
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				// 어빌리티 스펙의 InputPressed 플래그 설정
				AbilitySpec->InputPressed = true;

				if (AbilitySpec->IsActive()) // 핵심 분기점: 현재 활성화된 어빌리티인 경우
				{
					// 새로운 활성화 없이 입력 이벤트만 전달
					AbilitySpecInputPressed(*AbilitySpec);
				}
				else // 어빌리티가 비활성화된 경우
				{
					// 새로운 어빌리티 활성화 시도
					const USFGameplayAbility* SFAbilityCDO = Cast<USFGameplayAbility>(AbilitySpec->Ability);

					// OnInputTriggered 정책인 어빌리티만 입력으로 활성화 가능
					if (SFAbilityCDO && SFAbilityCDO->GetActivationPolicy() == ESFAbilityActivationPolicy::OnInputTriggered)
					{
						AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
					}
				}
			}
		}
	}

	//
	// 4단계: 큐잉된 모든 어빌리티들 일괄 활성화
	// 지속 입력과 눌림 입력을 한 번에 처리하여 중복 활성화 방지
	// 지속 입력이 어빌리티를 활성화한 후 눌림 입력이 같은 어빌리티에 입력 이벤트를 보내는 것을 방지
	//
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		// 실제 어빌리티 활성화 시도
		// 여기서 CanActivateAbility 체크 등 어빌리티 활성화 체크 로직이 실행됨
		TryActivateAbility(AbilitySpecHandle);
	}

	//
	// 5단계: 입력 릴리즈 이벤트 처리 (Released 이벤트)
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				// Ability Spec의 InputPressed 플래그 해제
				AbilitySpec->InputPressed = false;

				// 활성화된 어빌리티만 Release 이벤트를 전달
				if (AbilitySpec->IsActive())
				{
					// InputReleased 복제 이벤트 발생 (UAbilityTask_WaitInputRelease 등이 감지)
					AbilitySpecInputReleased(*AbilitySpec);
				}
			}
		}
	}

	//
	// 6단계: 캐시된 입력 핸들들 초기화
	// 다음 프레임을 위해 모든 입력 상태 배열을 리셋
	// InputHeldSpecHandles는 지속적으로 유지되므로 초기화하지 않음
	//
	InputStartedSpecHandles.Reset();
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void USFAbilitySystemComponent::ClearAbilityInput()
{
	InputStartedSpecHandles.Reset();
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

void USFAbilitySystemComponent::SaveAttributesToData(FSFSavedAbilitySystemData& OutData) const
{
	OutData.SavedAttributes.Reset();

	// 모든 Spawned AttributeSet 순회
	for (const UAttributeSet* Set : GetSpawnedAttributes())
	{
		if (!Set)
		{
			continue;
		}

		// AttributeSet의 모든 FGameplayAttributeData 프로퍼티 순회
		for (TFieldIterator<FProperty> It(Set->GetClass()); It; ++It)
		{
			FProperty* Property = *It;
			FStructProperty* StructProp = CastField<FStructProperty>(Property);
            
			if (!StructProp || StructProp->Struct != FGameplayAttributeData::StaticStruct())
			{
				continue;
			}

			FGameplayAttribute Attribute(Property);
			if (!Attribute.IsValid())
			{
				continue;
			}

			FSFSavedAttribute SavedAttr;
			SavedAttr.Attribute = Attribute;
			SavedAttr.BaseValue = GetNumericAttributeBase(Attribute);
			OutData.SavedAttributes.Add(SavedAttr);
		}
	}
}

void USFAbilitySystemComponent::RestoreAttributesFromData(const FSFSavedAbilitySystemData& InData)
{
	if (!InData.HasSavedAttributes())
	{
		return;
	}

	// Max 계열 Attribute를 먼저 복원하기 위해 분류
	TArray<const FSFSavedAttribute*> MaxAttributes;
	TArray<const FSFSavedAttribute*> OtherAttributes;

	for (const FSFSavedAttribute& SavedAttr : InData.SavedAttributes)
	{
		if (!SavedAttr.Attribute.IsValid())
		{
			continue;
		}

		// Attribute 이름에 Max가 포함되어 있으면 Max 계열로 분류
		const FString AttrName = SavedAttr.Attribute.GetName();
		if (AttrName.Contains(TEXT("Max")))
		{
			MaxAttributes.Add(&SavedAttr);
		}
		else
		{
			OtherAttributes.Add(&SavedAttr);
		}
	}

	int32 RestoredCount = 0;
	
	// Max 계열 먼저 복원 (MaxHealth, MaxMana, MaxStamina 등)
	for (const FSFSavedAttribute* SavedAttr : MaxAttributes)
	{
		SetNumericAttributeBase(SavedAttr->Attribute, SavedAttr->BaseValue);
		RestoredCount++;
	}

	// 나머지 Attribute 복원 (Health, Mana, Stamina 등)
	for (const FSFSavedAttribute* SavedAttr : OtherAttributes)
	{
		SetNumericAttributeBase(SavedAttr->Attribute, SavedAttr->BaseValue);
		RestoredCount++;
	}

	UE_LOG(LogSF, Log, TEXT("RestoreAttributesFromData: Restored %d/%d attributes"), RestoredCount, InData.SavedAttributes.Num());
}

void USFAbilitySystemComponent::SaveAbilitiesToData(FSFSavedAbilitySystemData& OutData) const
{
	OutData.SavedAbilities.Reset();

	FScopedAbilityListLock ActiveScopeLock(*const_cast<USFAbilitySystemComponent*>(this));
	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (!Spec.Ability)
		{
			continue;
		}

		// 저장 대상이 아닌 어빌리티는 스킵
		if (const USFGameplayAbility* SFAbility = Cast<USFGameplayAbility>(Spec.Ability))
		{
			if (!SFAbility->ShouldPersistOnTravel())
			{
				continue;
			}
		}

		if (Spec.GetDynamicSpecSourceTags().HasTag(SFGameplayTags::Ability_Skill_NonPersistent))
		{
			continue;
		}
		
		FSFSavedAbility SavedAbility;
		SavedAbility.AbilityClass = Spec.Ability->GetClass();
		SavedAbility.AbilityLevel = Spec.Level;
		SavedAbility.DynamicTags = Spec.GetDynamicSpecSourceTags();

		if (const USFGameplayAbility* Instance = Cast<USFGameplayAbility>(Spec.GetPrimaryInstance()))
		{
			if (Instance->HasCustomPersistentData())
			{
				SavedAbility.CustomData = Instance->SaveCustomPersistentData();
			}
		}

		OutData.SavedAbilities.Add(SavedAbility); 
	}

	UE_LOG(LogSF, Log, TEXT("SaveAbilitiesToData: Saved %d abilities"), OutData.SavedAbilities.Num());
}

void USFAbilitySystemComponent::RestoreAbilitiesFromData(const FSFSavedAbilitySystemData& InData)
{
	if (!InData.HasSavedAbilities())
	{
		return;
	}

	ClearAllAbilities();

	for (const FSFSavedAbility& SavedAbility : InData.SavedAbilities)
	{
		if (!SavedAbility.AbilityClass)
		{
			continue;
		}

		UGameplayAbility* AbilityCDO = SavedAbility.AbilityClass->GetDefaultObject<UGameplayAbility>();
		FGameplayAbilitySpec NewSpec(AbilityCDO, SavedAbility.AbilityLevel);
		NewSpec.GetDynamicSpecSourceTags().AppendTags(SavedAbility.DynamicTags);
		FGameplayAbilitySpecHandle Handle = GiveAbility(NewSpec);

		// 커스텀 데이터 복원
		if (SavedAbility.CustomData.IsValid())
		{
			if (FGameplayAbilitySpec* GrantedSpec = FindAbilitySpecFromHandle(Handle))
			{
				if (USFGameplayAbility* Instance = Cast<USFGameplayAbility>(GrantedSpec->GetPrimaryInstance()))
				{
					Instance->RestoreCustomPersistentData(SavedAbility.CustomData);
				}
			}
		}
	}
}

void USFAbilitySystemComponent::SaveGameplayEffectsToData(FSFSavedAbilitySystemData& OutData) const
{
	OutData.SavedGameplayEffects.Reset();
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Seamless Travel시 World의 InitializeActorsForPlay에서 World의 TimeSeconds를 기존 월드에 맞춤
	const float CurrentWorldTime = World->GetTimeSeconds();
	
	// 활성화된 모든 GE 순회
	FScopedActiveGameplayEffectLock ScopeLockActiveGameplayEffects(const_cast<FActiveGameplayEffectsContainer&>(ActiveGameplayEffects));
	for (const FActiveGameplayEffect& ActiveEffect : &ActiveGameplayEffects)
	{
		const UGameplayEffect* EffectDef = ActiveEffect.Spec.Def;
		if (!EffectDef)
		{
			continue;
		}

		// Instant는 저장 안 함
		if (EffectDef->DurationPolicy == EGameplayEffectDurationType::Instant)
		{
			continue;
		}

		// Infinite GE: Persist 태그가 있는 것만 저장
		if (EffectDef->DurationPolicy == EGameplayEffectDurationType::Infinite)
		{
			if (!EffectDef->GetAssetTags().HasTag(SFGameplayTags::Effect_Persist_OnTravel))
			{
				continue;
			}
		}

		// Duration 타입만 저장
		// 남은 시간 계산 EndTime - CurrentWorldTime
		const float RemainingTime = ActiveEffect.GetTimeRemaining(CurrentWorldTime);

		if (EffectDef->DurationPolicy == EGameplayEffectDurationType::HasDuration && RemainingTime <= 0.f)
		{
			continue;
		}

		FSFSavedGameplayEffect SavedEffect;
		SavedEffect.EffectClass = EffectDef->GetClass();
		SavedEffect.EffectLevel = ActiveEffect.Spec.GetLevel();
		SavedEffect.RemainingDuration = RemainingTime;
		SavedEffect.StackCount = ActiveEffect.Spec.GetStackCount();

		OutData.SavedGameplayEffects.Add(SavedEffect);
	}

	UE_LOG(LogSF, Log, TEXT("SaveGameplayEffectsToData: Saved %d duration effects"), 
		OutData.SavedGameplayEffects.Num());
}

void USFAbilitySystemComponent::RestoreGameplayEffectsFromData(const FSFSavedAbilitySystemData& InData)
{
	if (!InData.HasSavedEffects())
	{
		return;
	}

	int32 RestoredCount = 0;

	for (const FSFSavedGameplayEffect& SavedEffect : InData.SavedGameplayEffects)
	{
		if (!SavedEffect.EffectClass)
		{
			continue;
		}

		const UGameplayEffect* EffectCDO = SavedEffect.EffectClass->GetDefaultObject<UGameplayEffect>();
		if (!EffectCDO)
		{
			continue;
		}
	
		// HasDuration인데 시간이 다 된 경우만 스킵
		if (EffectCDO->DurationPolicy == EGameplayEffectDurationType::HasDuration && SavedEffect.RemainingDuration <= 0.f)
		{
			continue;
		}


		FGameplayEffectContextHandle EffectContext = MakeEffectContext();
		EffectContext.AddSourceObject(GetOwner());

		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(
			SavedEffect.EffectClass, 
			SavedEffect.EffectLevel, 
			EffectContext
		);

		if (!SpecHandle.IsValid())
		{
			continue;
		}

		FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

		// SetDuration으로 남은 시간만큼만 지속되도록 설정
		// LockDuration을 true로 하여 다른 곳에서 변경하지 못하게 함
		if (EffectCDO->DurationPolicy == EGameplayEffectDurationType::HasDuration)
		{
			Spec->SetDuration(SavedEffect.RemainingDuration, true);
		}

		// 스택 설정 (스택 가능한 GE인 경우)
		if (SavedEffect.StackCount > 1)
		{
			Spec->SetStackCount(SavedEffect.StackCount);
		}

		FActiveGameplayEffectHandle AppliedHandle = ApplyGameplayEffectSpecToSelf(*Spec);
		if (AppliedHandle.IsValid())
		{
			RestoredCount++;
		}
	}

	UE_LOG(LogSF, Log, TEXT("RestoreGameplayEffectsFromData: Restored %d/%d effects"),
		RestoredCount, InData.SavedGameplayEffects.Num());
}

void USFAbilitySystemComponent::CancelActiveAbilities(const FGameplayTagContainer* WithTags, const FGameplayTagContainer* WithoutTags, UGameplayAbility* Ignore, bool bIncludeOnSpawn)
{
	ABILITYLIST_SCOPE_LOCK();
	
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (!Spec.IsActive())
		{
			continue;
		}
		
		// OnSpawn 어빌리티는 스킵
		if (const USFGameplayAbility* SFAbility = Cast<USFGameplayAbility>(Spec.Ability))
		{
			if (SFAbility->GetActivationPolicy() == ESFAbilityActivationPolicy::OnSpawn)
			{
				// bIncludeOnSpawn이 false면 스킵
				if (!bIncludeOnSpawn)
				{
					continue;
				}
			}
		}

		// 태그 필터링
		bool bCancel = true;
		if (WithTags && !Spec.Ability->AbilityTags.HasAny(*WithTags))
		{
			bCancel = false;
		}
		if (WithoutTags && Spec.Ability->AbilityTags.HasAny(*WithoutTags))
		{
			bCancel = false;
		}

		if (bCancel)
		{
			CancelAbilitySpec(Spec, Ignore);
		}
	}
}
