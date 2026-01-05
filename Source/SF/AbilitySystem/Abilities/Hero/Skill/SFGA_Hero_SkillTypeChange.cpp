#include "SFGA_Hero_SkillTypeChange.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "SFLogChannels.h"
#include "Input/SFInputGameplayTags.h"

USFGA_Hero_SkillTypeChange::USFGA_Hero_SkillTypeChange()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	CurrentSetIndex = -1;
	bHasActivatedOnce = false;
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
          // 첫 번째 세트 설정
          CurrentSetIndex = 0;
          
          const USFAbilitySet* InitialSet = AbilitySets[0];
          FGameplayTag InitialElementTag;
          if (ElementTags.IsValidIndex(0))
          {
             InitialElementTag = ElementTags[0];
          }

          // 오버라이드 로직을 통해 부여
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

void USFGA_Hero_SkillTypeChange::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	// 이 관리자 스킬이 제거되면, 부여했던 하위 스킬들도 모두 회수
	USFAbilitySystemComponent* SFASC = Cast<USFAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
	if (SFASC) // IsValid 체크 필요 (SFAbilitySet.h 참고)
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

	// 1. 오버라이드 정보 저장
	FSFSkillOverrideInfo& Info = ElementSkillOverrides.FindOrAdd(ElementTag);
	Info.SlotOverrides.Add(InputTag, NewAbilityClass);

	UE_LOG(LogSF, Log, TEXT("[SkillTypeChange] Override Registered: Element[%s] Slot[%s] -> Class[%s]"), 
		*ElementTag.ToString(), *InputTag.ToString(), *GetNameSafe(NewAbilityClass));

	// 2. 만약 현재 사용 중인 속성이 방금 변경된 속성이라면, 즉시 새로고침(Refresh)
	if (ElementTags.IsValidIndex(CurrentSetIndex) && ElementTags[CurrentSetIndex] == ElementTag)
	{
		USFAbilitySystemComponent* SFASC = Cast<USFAbilitySystemComponent>(GetActorInfo().AbilitySystemComponent.Get());
		if (SFASC && AbilitySets.IsValidIndex(CurrentSetIndex))
		{
			// 기존 것 회수
			ActiveGrantedHandles.TakeFromAbilitySystem(SFASC);
			
			// 변경 사항 적용하여 다시 부여
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

	// 1. 기존 스킬 회수
	ActiveGrantedHandles.TakeFromAbilitySystem(SFASC);

	// 2. 인덱스 순환
	if (AbilitySets.Num() > 0)
	{
		CurrentSetIndex = (CurrentSetIndex + 1) % AbilitySets.Num();

		const USFAbilitySet* NextSet = AbilitySets[CurrentSetIndex];
		
		FGameplayTag CurrentElementTag;
		if (ElementTags.IsValidIndex(CurrentSetIndex))
		{
			CurrentElementTag = ElementTags[CurrentSetIndex];
		}

		// 3. 오버라이드 적용 부여
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

	// 1. Attribute Sets 부여 (기존 로직 동일)
	for (const FSFAbilitySet_AttributeSet& SetToGrant : AbilitySet->GetGrantedAttributes())
	{
		if (!IsValid(SetToGrant.AttributeSet)) continue;

		UAttributeSet* NewSet = NewObject<UAttributeSet>(SFASC->GetOwner(), SetToGrant.AttributeSet);
		SFASC->AddAttributeSetSubobject(NewSet);
		ActiveGrantedHandles.AddAttributeSet(NewSet);
	}

	// 2. Gameplay Abilities 부여 (오버라이드 로직 적용)
	for (const FSFAbilitySet_GameplayAbility& AbilityToGrant : AbilitySet->GetGrantedGameplayAbilities())
	{
		if (!IsValid(AbilityToGrant.Ability)) continue;

		TSubclassOf<USFGameplayAbility> AbilityClass = AbilityToGrant.Ability;
		FGameplayTag InputTag = AbilityToGrant.InputTag;

		// [핵심] 오버라이드 체크
		if (ElementSkillOverrides.Contains(CurrentElementTag))
		{
			const FSFSkillOverrideInfo& Info = ElementSkillOverrides[CurrentElementTag];
			if (Info.SlotOverrides.Contains(InputTag))
			{
				// 교체할 스킬이 있다면 그걸로 바꿈
				AbilityClass = Info.SlotOverrides[InputTag];
			}
		}

		if (!IsValid(AbilityClass)) continue;

		USFGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<USFGameplayAbility>();
		FGameplayAbilitySpec AbilitySpec(AbilityCDO, AbilityToGrant.AbilityLevel);
		AbilitySpec.SourceObject = SourceObject;
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(InputTag);

		const FGameplayAbilitySpecHandle Handle = SFASC->GiveAbility(AbilitySpec);
		ActiveGrantedHandles.AddAbilitySpecHandle(Handle);
	}

	// 3. Gameplay Effects 부여 (기존 로직 동일)
	for (const FSFAbilitySet_GameplayEffect& EffectToGrant : AbilitySet->GetGrantedGameplayEffects())
	{
		if (!IsValid(EffectToGrant.GameplayEffect)) continue;

		const UGameplayEffect* GameplayEffect = EffectToGrant.GameplayEffect->GetDefaultObject<UGameplayEffect>();
		const FActiveGameplayEffectHandle Handle = SFASC->ApplyGameplayEffectToSelf(GameplayEffect, EffectToGrant.EffectLevel, SFASC->MakeEffectContext());
		ActiveGrantedHandles.AddGameplayEffectHandle(Handle);
	}
}