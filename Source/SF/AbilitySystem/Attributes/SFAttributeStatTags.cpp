#include "SFAttributeStatTags.h"

namespace SFGameplayTags
{
	// Primary Stats
	UE_DEFINE_GAMEPLAY_TAG(Stat_MaxHealth, "Stat.MaxHealth");
	UE_DEFINE_GAMEPLAY_TAG(Stat_MaxMana, "Stat.MaxMana");
	UE_DEFINE_GAMEPLAY_TAG(Stat_MaxStamina, "Stat.MaxStamina");
	UE_DEFINE_GAMEPLAY_TAG(Stat_MoveSpeed, "Stat.MoveSpeed");

	// Combat Stats
	UE_DEFINE_GAMEPLAY_TAG(Stat_AttackPower, "Stat.AttackPower");
	UE_DEFINE_GAMEPLAY_TAG(Stat_Defense, "Stat.Defense");
	UE_DEFINE_GAMEPLAY_TAG(Stat_AttackSpeed, "Stat.AttackSpeed");
	UE_DEFINE_GAMEPLAY_TAG(Stat_CriticalChance, "Stat.CriticalChance");
	UE_DEFINE_GAMEPLAY_TAG(Stat_CriticalDamage, "Stat.CriticalDamage");

	// Hero Stats
	UE_DEFINE_GAMEPLAY_TAG(Stat_Luck, "Stat.Luck");
	UE_DEFINE_GAMEPLAY_TAG(Stat_CooldownRate, "Stat.CooldownRate");
	UE_DEFINE_GAMEPLAY_TAG(Stat_ManaRegen, "Stat.ManaRegen");
	UE_DEFINE_GAMEPLAY_TAG(Stat_StaminaRegen, "Stat.StaminaRegen");
	UE_DEFINE_GAMEPLAY_TAG(Stat_ManaReduction, "Stat.ManaReduction");
}