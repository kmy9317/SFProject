// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"


namespace SFGameplayTags
{
	// ========== Enemy Attack Ability Tags ==========

	// Light Attack (빠른 약공)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Attack_Light);

	// Heavy Attack (강공)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Attack_Heavy);

	// Charge Attack (돌진)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Attack_Charge);

	// Guard Break Attack (가드 파괴)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Attack_GuardBreak);

	// Grab Attack (잡기)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Attack_Grab);

	// Range Attack (원거리)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Attack_Range);

	// AoE Attack (범위)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Attack_AoE);


	// ========== Cooldown Tags ==========

	// Global Attack Cooldown
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Enemy_Attack);

	// Individual Cooldowns
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Enemy_Attack_Light);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Enemy_Attack_Heavy);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Enemy_Attack_Charge);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Enemy_Attack_GuardBreak);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Enemy_Attack_Grab);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Enemy_Attack_Range);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Enemy_Attack_AoE);


	
	// Hero Skill Cooldowns
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Hero_Skill_Primary);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Hero_Skill_Secondary);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Hero_Skill_Identity);
}
