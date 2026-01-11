#include "SFGA_ApplyTagEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"

USFGA_ApplyTagEffect::USFGA_ApplyTagEffect()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

// 1. 어빌리티를 획득했을 때 (태그 부착)
void USFGA_ApplyTagEffect::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	// ASC 확보
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC) return;

	// [핵심] 어빌리티 보유 시 무조건 태그 부착
	if (TagToAdd.IsValid())
	{
		ASC->AddLooseGameplayTag(TagToAdd);
	}

	// --- 기존 자동 발동 로직 ---
	if (Spec.IsActive()) return;

	if (ActorInfo->IsNetAuthority())
	{
		ASC->TryActivateAbility(Spec.Handle);
	}
}

// 2. 어빌리티를 잃었을 때 (태그 제거)
void USFGA_ApplyTagEffect::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	// 부모 로직 먼저 호출 (필요한 정리 작업)
	Super::OnRemoveAbility(ActorInfo, Spec);

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC) return;

	// [핵심] 어빌리티 제거 시 태그도 같이 제거
	if (TagToAdd.IsValid())
	{
		ASC->RemoveLooseGameplayTag(TagToAdd);
	}
}

void USFGA_ApplyTagEffect::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 태그 제거 로직 (기존)
	if (TagToRemove.IsValid())
	{
		FGameplayTagContainer TagsToRemoveContainer;
		TagsToRemoveContainer.AddTag(TagToRemove);
		ASC->RemoveActiveEffectsWithGrantedTags(TagsToRemoveContainer);
	}

	// 2. 지정된 이펙트 부여 로직 (기존)
	if (TargetGameplayEffectClass)
	{
		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddSourceObject(this);
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(TargetGameplayEffectClass, EffectLevel, ContextHandle);

		if (SpecHandle.IsValid())
		{
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}