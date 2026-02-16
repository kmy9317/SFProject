#include "SFGA_Hero_SkillTypeChange.h"

#include "SFHeroSkillTags.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "SFLogChannels.h"
#include "Input/SFInputGameplayTags.h"

USFGA_Hero_SkillTypeChange::USFGA_Hero_SkillTypeChange()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	CurrentSetIndex = -1;
	bHasActivatedOnce = false;
	bHasRestoredData = false;
}

void USFGA_Hero_SkillTypeChange::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	if (!ActorInfo->IsNetAuthority() || bHasActivatedOnce)
	{
		return;
	}

	if (AbilitySets.Num() == 0)
	{
		return;
	}

	USFAbilitySystemComponent* SFASC = Cast<USFAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
	if (!SFASC)
	{
		return;
	}

	// 인덱스 결정: 복원 데이터가 있으면 복원값, 없으면 0
	if (bHasRestoredData)
	{
		CurrentSetIndex = FMath::Clamp(CurrentSetIndex, 0, AbilitySets.Num() - 1);
		UE_LOG(LogSF, Log, TEXT("[SkillTypeChange] Using restored index: %d"), CurrentSetIndex);
	}
	else
	{
		CurrentSetIndex = 0;
	}

	// 어빌리티 세트 부여
	ApplyCurrentAbilitySet(SFASC);

	bHasActivatedOnce = true;
}

void USFGA_Hero_SkillTypeChange::ApplyCurrentAbilitySet(USFAbilitySystemComponent* SFASC)
{
	if (!AbilitySets.IsValidIndex(CurrentSetIndex))
	{
		return;
	}

	const USFAbilitySet* TargetSet = AbilitySets[CurrentSetIndex];
	FGameplayTag TargetElementTag;

	if (ElementTags.IsValidIndex(CurrentSetIndex))
	{
		TargetElementTag = ElementTags[CurrentSetIndex];
	}

	GiveAbilitySetWithOverrides(TargetSet, TargetElementTag);
}

FInstancedStruct USFGA_Hero_SkillTypeChange::SaveCustomPersistentData() const
{
	// 저장 전 현재 슬롯 레벨 캐싱
	const_cast<USFGA_Hero_SkillTypeChange*>(this)->SyncCurrentSlotLevels();
    
	FSFSkillTypeChangeData Data;
	Data.CurrentSetIndex = CurrentSetIndex;
	Data.ElementOverrides = ElementSkillOverrides;
	Data.SharedSlotLevels = SharedSlotLevels; 
  
	return FInstancedStruct::Make(Data);
}

void USFGA_Hero_SkillTypeChange::RestoreCustomPersistentData(const FInstancedStruct& InData)
{
	if (const FSFSkillTypeChangeData* Data = InData.GetPtr<FSFSkillTypeChangeData>())
	{
		CurrentSetIndex = Data->CurrentSetIndex;
		ElementSkillOverrides = Data->ElementOverrides;
		SharedSlotLevels = Data->SharedSlotLevels;
		bHasRestoredData = true;
	}
}


void USFGA_Hero_SkillTypeChange::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 스킬 사용 시 다음 속성으로 교체
	CycleToNextAbilitySet();

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void USFGA_Hero_SkillTypeChange::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGA_Hero_SkillTypeChange::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	// 이 관리자 스킬이 제거되면, 부여했던 하위 스킬들도 모두 회수
	USFAbilitySystemComponent* SFASC = Cast<USFAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
	if (SFASC)
	{
		ActiveGrantedHandles.TakeFromAbilitySystem(SFASC);
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}

void USFGA_Hero_SkillTypeChange::RegisterSkillOverride(FGameplayTag ElementTag, FGameplayTag InputTag, TSubclassOf<USFGameplayAbility> NewAbilityClass)
{
	if (!ElementTag.IsValid() || !InputTag.IsValid() || !NewAbilityClass)
	{
		return;
	}

	USFAbilitySystemComponent* SFASC = Cast<USFAbilitySystemComponent>(GetActorInfo().AbilitySystemComponent.Get());

	// 클래스 오버라이드만 저장
	FSFSkillOverrideInfo& Info = ElementSkillOverrides.FindOrAdd(ElementTag);
	Info.SlotOverrides.Add(InputTag, NewAbilityClass);

	// 현재 사용 중인 속성이면 즉시 새로고침
	if (ElementTags.IsValidIndex(CurrentSetIndex) && ElementTags[CurrentSetIndex] == ElementTag)
	{
		if (SFASC && AbilitySets.IsValidIndex(CurrentSetIndex))
		{
			SwapManagedAbility(SFASC, InputTag, NewAbilityClass);
		}
	}
}

void USFGA_Hero_SkillTypeChange::CycleToNextAbilitySet()
{
	USFAbilitySystemComponent* SFASC = Cast<USFAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());
	if (!SFASC)
	{
		return;
	}
    
	// 현재 슬롯 레벨을 공유 레벨에 동기화 (속성 변경 전)
	SyncCurrentSlotLevels();
    
	// 기존 스킬 회수
	ActiveGrantedHandles.TakeFromAbilitySystem(SFASC);

	// 인덱스 순환
	CurrentSetIndex = (CurrentSetIndex + 1) % AbilitySets.Num();
	ApplyCurrentAbilitySet(SFASC);
}

void USFGA_Hero_SkillTypeChange::GiveAbilitySetWithOverrides(const USFAbilitySet* AbilitySet, FGameplayTag CurrentElementTag)
{
	if (!IsValid(AbilitySet))
	{
		return;
	}

	USFAbilitySystemComponent* SFASC = Cast<USFAbilitySystemComponent>(GetActorInfo().AbilitySystemComponent.Get());
	if (!SFASC)
	{
		return;
	}

	if (!SFASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	UObject* SourceObject = GetAvatarActorFromActorInfo();

	// 1. Attribute Sets 부여
    for (const FSFAbilitySet_AttributeSet& SetToGrant : AbilitySet->GetGrantedAttributes())
    {
	    if (!IsValid(SetToGrant.AttributeSet))
	    {
	    	continue;
		}
        UAttributeSet* NewSet = NewObject<UAttributeSet>(SFASC->GetOwner(), SetToGrant.AttributeSet);
        SFASC->AddAttributeSetSubobject(NewSet);
        ActiveGrantedHandles.AddAttributeSet(NewSet);
    }

    // 2. Gameplay Abilities 부여
    for (const FSFAbilitySet_GameplayAbility& AbilityToGrant : AbilitySet->GetGrantedGameplayAbilities())
    {
        if (!IsValid(AbilityToGrant.Ability))
        {
	        continue;
        }
        TSubclassOf<USFGameplayAbility> AbilityClass = AbilityToGrant.Ability;
        FGameplayTag InputTag = AbilityToGrant.InputTag;
        int32 AbilityLevel = AbilityToGrant.AbilityLevel;  // 기본값

        // 클래스 오버라이드 체크
        if (ElementSkillOverrides.Contains(CurrentElementTag))
        {
            const FSFSkillOverrideInfo& Info = ElementSkillOverrides[CurrentElementTag];
            if (Info.SlotOverrides.Contains(InputTag))
            {
                AbilityClass = Info.SlotOverrides[InputTag];
            }
        }

        // [핵심] 공유 레벨 적용
        if (SharedSlotLevels.Contains(InputTag))
        {
            AbilityLevel = SharedSlotLevels[InputTag];
        }

        if (!IsValid(AbilityClass))
        {
	        continue;
        }
        USFGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<USFGameplayAbility>();
        FGameplayAbilitySpec AbilitySpec(AbilityCDO, AbilityLevel);
        AbilitySpec.SourceObject = SourceObject;
        AbilitySpec.GetDynamicSpecSourceTags().AddTag(InputTag);

    	// 관리 대상 표시 → SeamlessTravel 시 저장 제외
    	AbilitySpec.GetDynamicSpecSourceTags().AddTag(SFGameplayTags::Ability_Skill_NonPersistent);
    	const FGameplayAbilitySpecHandle Handle = SFASC->GiveAbility(AbilitySpec);
        ActiveGrantedHandles.AddAbilitySpecHandle(Handle);
    }
    // 3. Gameplay Effects 부여
    for (const FSFAbilitySet_GameplayEffect& EffectToGrant : AbilitySet->GetGrantedGameplayEffects())
    {
        if (!IsValid(EffectToGrant.GameplayEffect)) continue;

        const UGameplayEffect* GameplayEffect = EffectToGrant.GameplayEffect->GetDefaultObject<UGameplayEffect>();
        const FActiveGameplayEffectHandle Handle = SFASC->ApplyGameplayEffectToSelf(GameplayEffect, EffectToGrant.EffectLevel, SFASC->MakeEffectContext());
        ActiveGrantedHandles.AddGameplayEffectHandle(Handle);
    }
}

void USFGA_Hero_SkillTypeChange::SyncCurrentSlotLevels()
{
	USFAbilitySystemComponent* SFASC = Cast<USFAbilitySystemComponent>(GetActorInfo().AbilitySystemComponent.Get());
	if (!SFASC)
	{
		return;
	}

	TArray<FGameplayTag> InputTagsToSync;
	InputTagsToSync.Add(SFGameplayTags::InputTag_PrimarySkill);
	InputTagsToSync.Add(SFGameplayTags::InputTag_SecondarySkill);
	InputTagsToSync.Add(SFGameplayTags::InputTag_Attack);

	for (const FGameplayTag& InputTag : InputTagsToSync)
	{
		for (const FGameplayAbilitySpec& Spec : SFASC->GetActivatableAbilities())
		{
			if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			{
				// 공유 레벨에 저장 (현재 ASC의 레벨이 가장 최신)
				SharedSlotLevels.Add(InputTag, Spec.Level);
				break;
			}
		}
	}
}

void USFGA_Hero_SkillTypeChange::SwapManagedAbility(USFAbilitySystemComponent* SFASC, FGameplayTag InputTag, TSubclassOf<USFGameplayAbility> NewAbilityClass)
{
	for (const FGameplayAbilitySpec& Spec : SFASC->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag) && Spec.GetDynamicSpecSourceTags().HasTag(SFGameplayTags::Ability_Skill_NonPersistent))
		{
			int32 PrevLevel = Spec.Level;
			SharedSlotLevels.Add(InputTag, PrevLevel);

			ActiveGrantedHandles.RemoveAbilitySpecHandle(Spec.Handle);
			SFASC->ClearAbility(Spec.Handle);

			USFGameplayAbility* NewCDO = NewAbilityClass->GetDefaultObject<USFGameplayAbility>();
			FGameplayAbilitySpec NewSpec(NewCDO, PrevLevel);
			NewSpec.SourceObject = GetAvatarActorFromActorInfo();
			NewSpec.GetDynamicSpecSourceTags().AddTag(InputTag);
			NewSpec.GetDynamicSpecSourceTags().AddTag(SFGameplayTags::Ability_Skill_NonPersistent);

			FGameplayAbilitySpecHandle NewHandle = SFASC->GiveAbility(NewSpec);
			ActiveGrantedHandles.AddAbilitySpecHandle(NewHandle);
			break;
		}
	}
}
