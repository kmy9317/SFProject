#include "SFCombatTags.h"

namespace SFGameplayTags
{
	// ========== Combat Phase Tags ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Phase_Windup, "Combat.Phase.Windup", "Attack windup phase - direction change allowed");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Phase_Active, "Combat.Phase.Active", "Attack active phase - hitbox active, direction locked");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Phase_Recovery, "Combat.Phase.Recovery", "Attack recovery phase - input buffer active");

	// ========== Combat Phase Transition Events ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Event_Commit, "Combat.Event.Commit", "Transition from Windup to Active - direction committed");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Event_ReleaseComplete, "Combat.Event.ReleaseComplete", "Transition from Active to Recovery");
}