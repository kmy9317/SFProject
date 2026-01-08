#include "SFInputGameplayTags.h"

namespace SFGameplayTags
{
	// Native Input Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move", "Move input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Mouse, "InputTag.Look.Mouse", "Look (mouse) input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Crouch, "InputTag.Crouch", "Crouch input.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_UseQuickbar_1, "InputTag.UseQuickbar.1", "Use Quickbar Slot 1");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_UseQuickbar_2, "InputTag.UseQuickbar.2", "Use Quickbar Slot 2");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_UseQuickbar_3, "InputTag.UseQuickbar.3", "Use Quickbar Slot 3");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_UseQuickbar_4, "InputTag.UseQuickbar.4", "Use Quickbar Slot 4");

	// Ability Input Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_PrimarySkill, "InputTag.PrimarySkill", "Primary Skill input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_SecondarySkill, "InputTag.SecondarySkill", "Secondary Skill input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_IdentitySkill, "InputTag.IdentitySkill", "Identity Skill input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Attack, "InputTag.Attack", "Attack input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Dodge, "InputTag.Dodge", "Dodge input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Interact, "InputTag.Interact", "Interact input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_ToggleInventory, "InputTag.ToggleInventory", "Toggle Inventory.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Sprint, "InputTag.Sprint", "Sprint input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_LockOn, "InputTag.LockOn", "LockOn input.");

	// Dummy Data
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_AttackUpgrade, "InputTag.AttackUpgrade", "Upgrade Dummy.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_GroundUpgrade, "InputTag.GroundUpgrade", "Upgrade Dummy.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_UltUpgrade, "InputTag.UltUpgrade", "Upgrade Dummy.");
}