#include "SFGA_Hero_SkillTypeChange.h"
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

    // 초기화: 게임 시작 시 0번(첫 번째) 세트 부여
    if (ActorInfo->IsNetAuthority() && !bHasActivatedOnce)
    {
       // [추가된 로직] 초기화 전, 기존에 InputTag를 가진 스킬들을 청소 (방어 코드)
       USFAbilitySystemComponent* SFASC = Cast<USFAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
       if (SFASC)
       {
           // 삭제할 태그 목록 정의 (소서러가 사용하는 슬롯들)
           TArray<FGameplayTag> InputTagsToClear;
           InputTagsToClear.Add(SFGameplayTags::InputTag_PrimarySkill);
           InputTagsToClear.Add(SFGameplayTags::InputTag_SecondarySkill);
       	   InputTagsToClear.Add(SFGameplayTags::InputTag_Attack);
           // 필요한 태그 추가...

           // 삭제할 핸들을 임시 저장할 배열 (반복문 안에서 직접 삭제하면 충돌 위험)
           TArray<FGameplayAbilitySpecHandle> AbilitiesToRemove;

           // ASC의 모든 어빌리티 순회
           for (const FGameplayAbilitySpec& AbilitySpec : SFASC->GetActivatableAbilities())
           {
               // 현재 이 관리자 스킬(Self)은 삭제하면 안 됨!
               if (AbilitySpec.Ability == this)
               {
                   continue;
               }

               // 해당 어빌리티가 우리가 지우려는 InputTag를 가지고 있는지 검사
               for (const FGameplayTag& TagToCheck : InputTagsToClear)
               {
                   if (AbilitySpec.GetDynamicSpecSourceTags().HasTag(TagToCheck))
                   {
                       AbilitiesToRemove.Add(AbilitySpec.Handle);
                       break; // 태그 하나만 걸려도 삭제 대상
                   }
               }
           }

           // 수집된 어빌리티 일괄 삭제
           for (const FGameplayAbilitySpecHandle& HandleToRemove : AbilitiesToRemove)
           {
               SFASC->ClearAbility(HandleToRemove);
           }
       }

       // --- 기존 초기화 로직 시작 ---
    	if (AbilitySets.Num() > 0)
    	{
    		// 복원된 데이터가 있으면 해당 인덱스 사용, 없으면 0
    		if (bHasRestoredData)
    		{
    			CurrentSetIndex = FMath::Clamp(CurrentSetIndex, 0, AbilitySets.Num() - 1);
    			UE_LOG(LogSF, Log, TEXT("[SkillTypeChange] Using restored index: %d"), CurrentSetIndex);
    		}
    		else
    		{
    			CurrentSetIndex = 0;
    		}
            
    		const USFAbilitySet* InitialSet = AbilitySets[CurrentSetIndex];
    		FGameplayTag InitialElementTag;
    		if (ElementTags.IsValidIndex(CurrentSetIndex))
    		{
    			InitialElementTag = ElementTags[CurrentSetIndex];
    		}

    		GiveAbilitySetWithOverrides(InitialSet, InitialElementTag);
            
    		bHasActivatedOnce = true;
    	}
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

FInstancedStruct USFGA_Hero_SkillTypeChange::SaveCustomPersistentData() const
{
	// 저장 전 현재 슬롯 레벨 캐싱 (const 우회)
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

		// TODO : 삭제 예정
		if (bHasActivatedOnce)
		{
			USFAbilitySystemComponent* SFASC = Cast<USFAbilitySystemComponent>(GetActorInfo().AbilitySystemComponent.Get());
			if (SFASC && AbilitySets.IsValidIndex(CurrentSetIndex))
			{
				ActiveGrantedHandles.TakeFromAbilitySystem(SFASC);
                
				const USFAbilitySet* TargetSet = AbilitySets[CurrentSetIndex];
				FGameplayTag TargetElementTag;
				if (ElementTags.IsValidIndex(CurrentSetIndex))
				{
					TargetElementTag = ElementTags[CurrentSetIndex];
				}
                
				GiveAbilitySetWithOverrides(TargetSet, TargetElementTag);
			}
		}
	}
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
    
	// 현재 슬롯 레벨 동기화 (현재 활성화된 스킬 레벨을 공유 레벨에 반영)
	if (ElementTags.IsValidIndex(CurrentSetIndex) && ElementTags[CurrentSetIndex] == ElementTag)
	{
		int32 CurrentLevel = GetCurrentSlotLevel(InputTag);
		SharedSlotLevels.Add(InputTag, CurrentLevel);
	}

	// 클래스 오버라이드만 저장
	FSFSkillOverrideInfo& Info = ElementSkillOverrides.FindOrAdd(ElementTag);
	Info.SlotOverrides.Add(InputTag, NewAbilityClass);

	// 현재 사용 중인 속성이면 즉시 새로고침
	if (ElementTags.IsValidIndex(CurrentSetIndex) && ElementTags[CurrentSetIndex] == ElementTag)
	{
		if (SFASC && AbilitySets.IsValidIndex(CurrentSetIndex))
		{
			SyncCurrentSlotLevels();
			ActiveGrantedHandles.TakeFromAbilitySystem(SFASC);
			GiveAbilitySetWithOverrides(AbilitySets[CurrentSetIndex], ElementTag);
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
	if (AbilitySets.Num() > 0)
	{
		CurrentSetIndex = (CurrentSetIndex + 1) % AbilitySets.Num();

		const USFAbilitySet* NextSet = AbilitySets[CurrentSetIndex];
        
		FGameplayTag CurrentElementTag;
		if (ElementTags.IsValidIndex(CurrentSetIndex))
		{
			CurrentElementTag = ElementTags[CurrentSetIndex];
		}

		GiveAbilitySetWithOverrides(NextSet, CurrentElementTag);
	}
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

        const FGameplayAbilitySpecHandle Handle = SFASC->GiveAbility(AbilitySpec);
        ActiveGrantedHandles.AddAbilitySpecHandle(Handle);
        
        UE_LOG(LogSF, Verbose, TEXT("[SkillTypeChange] Granted: %s (Level %d) for slot %s"), 
            *GetNameSafe(AbilityClass), AbilityLevel, *InputTag.ToString());
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

int32 USFGA_Hero_SkillTypeChange::GetCurrentSlotLevel(FGameplayTag InputTag) const
{
	USFAbilitySystemComponent* SFASC = Cast<USFAbilitySystemComponent>(GetActorInfo().AbilitySystemComponent.Get());
	if (!SFASC)
	{
		return 1;
	}

	for (const FGameplayAbilitySpec& Spec : SFASC->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			return Spec.Level;
		}
	}

	return 1;
}
