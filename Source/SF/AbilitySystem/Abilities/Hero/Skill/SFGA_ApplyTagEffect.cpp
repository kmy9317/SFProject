#include "SFGA_ApplyTagEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"

USFGA_ApplyTagEffect::USFGA_ApplyTagEffect()
{
	// 인스턴싱 정책: 상태 저장이 필요 없다면 NonInstanced가 가볍지만,
	// 안전하게 실행하려면 InstancedPerActor를 유지해도 됩니다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 자동 발동이므로 Trigger 별도 설정 불필요 (코드에서 직접 호출함)
	// ActivationPolicy = ESFAbilityActivationPolicy::OnInputTriggered; // 필요 시 주석 처리
}

// [핵심 로직] 어빌리티가 Actor에게 주어졌을 때(GiveAbility 시점) 호출됨
void USFGA_ApplyTagEffect::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	// 이미 활성화 상태라면 패스
	if (Spec.IsActive())
	{
		return;
	}

	// 권한이 있는 곳(주로 서버)에서만 실행하여 이펙트 적용/태그 제거를 수행
	if (ActorInfo->IsNetAuthority())
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		if (ASC)
		{
			// 즉시 어빌리티 실행 시도 -> ActivateAbility 호출됨
			ASC->TryActivateAbility(Spec.Handle);
		}
	}
}

void USFGA_ApplyTagEffect::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 커밋 (쿨타임/코스트 확인)
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

	// 2. 태그 제거 로직
	if (TagToRemove.IsValid())
	{
		FGameplayTagContainer TagsToRemoveContainer;
		TagsToRemoveContainer.AddTag(TagToRemove);
		ASC->RemoveActiveEffectsWithGrantedTags(TagsToRemoveContainer);
	}

	// 3. 지정된 이펙트 부여 로직
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

	// 4. 어빌리티 종료 (즉시 발동 후 역할이 끝났으므로 종료)
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}