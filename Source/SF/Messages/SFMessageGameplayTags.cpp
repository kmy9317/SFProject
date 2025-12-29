#include "SFMessageGameplayTags.h"

namespace SFGameplayTags
{
	// Gameplay Message Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Portal_StateChanged, "Message.Portal.InfoChanged", "Portal Info Changed Message");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Player_TravelReadyChanged, "Message.Player.TravelReadyChanged", "Player Travel Ready Changed Message");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Skill_ProgressInfoChanged, "Message.Skill.ProgressInfoChanged", "Skill Progress Info Changed Message");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Skill_ProgressRefresh, "Message.Skill.ProgressRefresh", "Skill Progress Refresh Message");
	UE_DEFINE_GAMEPLAY_TAG(Message_Skill_ChainStateChanged, "Message.Skill.ChainStateChanged");
	UE_DEFINE_GAMEPLAY_TAG(Message_Interaction_Notice, "Message.Interaction.Notice");
	UE_DEFINE_GAMEPLAY_TAG(Message_Interaction_Progress, "Message.Interaction.Progress");
	UE_DEFINE_GAMEPLAY_TAG(Message_Interaction_Interacting, "Message.Interaction.Interacting");
	UE_DEFINE_GAMEPLAY_TAG(Message_RewardSelection_Complete, "Message.RewardSelection.Complete");
	UE_DEFINE_GAMEPLAY_TAG(Message_Player_DeadStateChanged, "Message.Player.DeadStateChanged");
	UE_DEFINE_GAMEPLAY_TAG(Message_Player_DeadStateChangedUI, "Message.Player.DeadStateChangedUI");
	UE_DEFINE_GAMEPLAY_TAG(Message_Player_DownedStateChanged, "Message.Player.DownedStateChanged");
	UE_DEFINE_GAMEPLAY_TAG(Message_Player_DownedStateChangedUI, "Message.Player.DownedStateChangedUI");
	UE_DEFINE_GAMEPLAY_TAG(Message_Game_GameOver, "Message.Game.GameOver");
}