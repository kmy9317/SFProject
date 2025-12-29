#pragma once

#include "NativeGameplayTags.h"

namespace SFGameplayTags
{
	// Hero Skill Tags
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Identity_Hero);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Primary_Hero);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Secondary_Hero);

	// Hero Upgrade Tags
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Passive_FreeReroll);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Passive_MoreEnhance);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Passive_CooldownReset);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Passive_LastStand);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Passive_LastStand_Use);
}