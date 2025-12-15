#include "SFGA_Hero_ForceComboAttack.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"

void USFGA_Hero_ForceComboAttack::CompleteCombo(UGameplayAbility* SourceAbility)
{
	Super::CompleteCombo(SourceAbility);

	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	if (!ReplaceAbilityClass || !ReplaceInputTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}
	
	//강화 되었던 슬롯 GA 제거 + 레벨 계승
	int32 InheritedLevel = 1;

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(ReplaceInputTag))
		{
			InheritedLevel = Spec.Level;
			ASC->ClearAbility(Spec.Handle);
			break;
		}
	}
	
	//원래 GA 부여
	USFGameplayAbility* AbilityCDO =
		ReplaceAbilityClass->GetDefaultObject<USFGameplayAbility>();

	FGameplayAbilitySpec NewSpec(AbilityCDO, InheritedLevel);
	NewSpec.GetDynamicSpecSourceTags().AddTag(ReplaceInputTag);

	ASC->GiveAbility(NewSpec);

	UE_LOG(LogTemp, Warning,
		TEXT("[ParryComboAttack] Combo complete → Ability replaced (%s)"),
		*GetNameSafe(ReplaceAbilityClass));
}

void USFGA_Hero_ForceComboAttack::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	RemoveComboState(this);
}
