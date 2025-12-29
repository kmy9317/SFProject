#include "SFInputGameplayTags.h"

namespace SFGameplayTags
{
	// Native Input Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move", "Move input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Mouse, "InputTag.Look.Mouse", "Look (mouse) input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Crouch, "InputTag.Crouch", "Crouch input.");

	// Ability Input Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_PrimarySkill, "InputTag.PrimarySkill", "Primary Skill input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_SecondarySkill, "InputTag.SecondarySkill", "Secondary Skill input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_IdentitySkill, "InputTag.IdentitySkill", "Identity Skill input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Attack, "InputTag.Attack", "Attack input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Dodge, "InputTag.Dodge", "Dodge input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Interact, "InputTag.Interact", "Interact input.");
}