#include "SFGA_Hero_Parrying_C.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"

void USFGA_Hero_Parrying_C::OnParrySuccess(const FGameplayEventData& Payload, AActor* InstigatorActor)
{
	ReplaceAbilityOnServer();
}

void USFGA_Hero_Parrying_C::ReplaceAbilityOnServer()
{
	if (!ReplacementAbilityClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Parry_C] ReplacementAbilityClass is NULL"));
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	FGameplayAbilitySpec* CurrentSpec = ASC->FindAbilitySpecFromHandle(CurrentSpecHandle);
	if (!CurrentSpec)
	{
		return;
	}

	const int32 AbilityLevel = CurrentSpec->Level;
	
	//InputTag에 해당하는 GA 제거
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(ReplacementInputTag))
		{
			ASC->ClearAbility(Spec.Handle);
			break;
		}
	}
	
	//지정한 강화 GA 부여
	USFGameplayAbility* AbilityCDO = ReplacementAbilityClass->GetDefaultObject<USFGameplayAbility>();
	FGameplayAbilitySpec NewSpec(AbilityCDO, AbilityLevel);
	NewSpec.GetDynamicSpecSourceTags().AddTag(ReplacementInputTag);
	
	ASC->GiveAbility(NewSpec);
}
