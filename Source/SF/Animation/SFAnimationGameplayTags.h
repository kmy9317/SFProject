#pragma once

#include "NativeGameplayTags.h"

namespace SFGameplayTags
{
	// Animation Montage Type Tags

	// Interact Tags
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Montage_Interact_Chest_Start);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Montage_Interact_Chest_End);

	// Equip Tags
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Montage_Equip_OneHandSword);

	// Consume Tags
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Montage_Consume_Potion);

	// State Tags
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Montage_State_Downed);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Montage_State_Death);
}