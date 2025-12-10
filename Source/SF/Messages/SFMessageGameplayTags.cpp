#include "SFMessageGameplayTags.h"

namespace SFGameplayTags
{
	// Gameplay Message Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Portal_StateChanged, "Message.Portal.InfoChanged", "Portal Info Changed Message");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Player_TravelReadyChanged, "Message.Player.TravelReadyChanged", "Player Travel Ready Changed Message");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Skill_ProgressInfoChanged, "Message.Skill.ProgressInfoChanged", "Skill Progress Info Changed Message");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Skill_ProgressRefresh, "Message.Skill.ProgressRefresh", "Skill Progress Refresh Message");
	UE_DEFINE_GAMEPLAY_TAG(Message_Skill_ChainStateChanged, "Message.Skill.ChainStateChanged");

}