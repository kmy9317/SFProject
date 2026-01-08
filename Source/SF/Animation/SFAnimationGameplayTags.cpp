#include "SFAnimationGameplayTags.h"

namespace SFGameplayTags
{
	// Interact Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Montage_Interact_Chest_Start, "Montage.Interact.Chest.Start", "Interaction chest start montage tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Montage_Interact_Chest_End, "Montage.Interact.Chest.End", "Interaction chest end montage tag");

	// Equip Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Montage_Equip_OneHandSword, "Montage.Equip.OneHandSword", "Equip one-handed sword montage tag");

	// Consume Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Montage_Consume_Potion, "Montage.Consume.Potion", "Consume potion montage tag");

	// State Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Montage_State_Downed, "Montage.State.Downed", "Downed state montage tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Montage_State_Death, "Montage.State.Death", "Death state montage tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Montage_State_Grabbed, "Montage.State.Grabbed", "Grabbed state montage tag");
}
