#include "SFGA_Hero_Parrying_A.h"

#include "AbilitySystemComponent.h"

void USFGA_Hero_Parrying_A::OnChainMontageCompleted()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		Super::OnChainMontageCompleted();
		return;
	}

	//부모 Parrying에서 부여한 태그로 패링 성공 여부 판별
	const bool bParrySuccess = ASC->HasMatchingGameplayTag(ParryEventTag);

	//실패 시 → 기존 Parrying 로직 그대로
	if (!bParrySuccess)
	{
		Super::OnChainMontageCompleted();
		return;
	}

	//패링 성공 시 쿨타임 90% 감소
	if (HasAuthority(&CurrentActivationInfo))
	{
		TSubclassOf<UGameplayEffect> CooldownClass = GetCompleteCooldownEffectClass();
		if (!CooldownClass)
		{
			CooldownClass = GetTimeoutCooldownEffectClass();
		}

		if (CooldownClass)
		{
			FGameplayEffectSpecHandle SpecHandle =
				MakeOutgoingGameplayEffectSpec(CooldownClass, GetAbilityLevel());

			if (SpecHandle.IsValid())
			{
				FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
				if (Spec)
				{
					const float OriginalDuration = Spec->GetDuration();
					const float Scale = FMath::Clamp(ParrySuccessCooldownScale, 0.0f, 1.0f);

					if (OriginalDuration > 0.f && OriginalDuration < BIG_NUMBER)
					{
						Spec->SetDuration(OriginalDuration * Scale, true);
					}

					ASC->ApplyGameplayEffectSpecToSelf(*Spec);
				}
			}
		}
	}

	//패링 성공 시 추가 GE(Ability 1회당 한번만 적용) - ★ 태그 방식
	if (HasAuthority(&CurrentActivationInfo)
		&& ParrySuccessExtraEffect
		&& ParryExtraEffectAppliedTag.IsValid()
		&& !ASC->HasMatchingGameplayTag(ParryExtraEffectAppliedTag))
	{
		FGameplayEffectSpecHandle ExtraSpecHandle =
			MakeOutgoingGameplayEffectSpec(ParrySuccessExtraEffect, GetAbilityLevel());

		if (ExtraSpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*ExtraSpecHandle.Data.Get());

			//1회 적용 마킹 태그 부여
			ASC->AddLooseGameplayTag(ParryExtraEffectAppliedTag);

			UE_LOG(LogTemp, Warning, TEXT("[Parry_A] Extra GE Applied ONCE (Tag)"));
		}
	}


	//태그 제거
	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		false,
		false
	);
}

void USFGA_Hero_Parrying_A::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	//태그 제거는 EndAbility에서 처리
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()
		&& ParryExtraEffectAppliedTag.IsValid())
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		ASC->RemoveLooseGameplayTag(ParryExtraEffectAppliedTag);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
