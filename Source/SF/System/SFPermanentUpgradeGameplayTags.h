#pragma once

#include "NativeGameplayTags.h"

namespace SFPermanentUpgradeTags
{
	//SetByCaller
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Upgrade_MaxHealthPct);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Upgrade_MaxManaPct);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Upgrade_MaxStaminaPct);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Upgrade_CritDamagePct);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Upgrade_AttackPowerPct);
	
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Upgrade_MaxHealth);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Upgrade_MaxMana);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Upgrade_MaxStamina);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Upgrade_Luck);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Upgrade_CritChance);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Upgrade_ManaRegen);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Upgrade_StaminaRegen);
	
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Upgrade_CooldownReductionPct);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Upgrade_CurrencyGainPct);

	//Passive Tier
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Upgrade_Wrath_Tier3);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Upgrade_Greed_Tier3);
}
