#include "SFInteractable.h"

#include "AbilitySystemComponent.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Player/SFPlayerState.h"


// Add default functionality here for any ISFInteractable functions that are not pure virtual.
bool ISFInteractable::CanInteraction(const FSFInteractionQuery& InteractionQuery) const
{
	AActor* RequestingAvatar = InteractionQuery.RequestingAvatar.Get();
	if (!RequestingAvatar)
	{
		return false;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(RequestingAvatar))
	{
		if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Downed))
		{
			return false;
		}
	}

	if (AController* Controller = InteractionQuery.RequestingController.Get())
	{
		if (ASFPlayerState* SFPS = Controller->GetPlayerState<ASFPlayerState>())
		{
			if (SFPS->IsDead())
			{
				return false;
			}
		}
	}

	return true;
}
