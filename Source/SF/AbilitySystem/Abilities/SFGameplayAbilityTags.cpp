// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGameplayAbilityTags.h"

namespace SFGameplayTags
{
	// ========== Enemy Attack Ability Tags ==========

	// Light Attack (빠른 약공)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Attack_Light, "Ability.Enemy.Attack.Light", "Light Attack - Fast weak attack");

	// Heavy Attack (강공)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Attack_Heavy, "Ability.Enemy.Attack.Heavy", "Heavy Attack - Slow strong attack");

	// Charge Attack (돌진)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Attack_Charge, "Ability.Enemy.Attack.Charge", "Charge Attack - Rush attack");

	// Guard Break Attack (가드 파괴)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Attack_GuardBreak, "Ability.Enemy.Attack.GuardBreak", "Guard Break Attack - Breaks player guard");

	// Grab Attack (잡기)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Attack_Grab, "Ability.Enemy.Attack.Grab", "Grab Attack - Unblockable grab");

	// Range Attack (원거리)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Attack_Range, "Ability.Enemy.Attack.Range", "Range Attack - Ranged attack");

	// AoE Attack (범위)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Attack_AoE, "Ability.Enemy.Attack.AoE", "AoE Attack - Area of Effect attack");


	// ========== Cooldown Tags ==========

	// Global Attack Cooldown
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Enemy_Attack, "Ability.Cooldown.Enemy.Attack", "Global cooldown for all enemy attacks");

	// Individual Cooldowns
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Enemy_Attack_Light, "Ability.Cooldown.Enemy.Attack.Light", "Cooldown for Light Attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Enemy_Attack_Heavy, "Ability.Cooldown.Enemy.Attack.Heavy", "Cooldown for Heavy Attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Enemy_Attack_Charge, "Ability.Cooldown.Enemy.Attack.Charge", "Cooldown for Charge Attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Enemy_Attack_GuardBreak, "Ability.Cooldown.Enemy.Attack.GuardBreak", "Cooldown for Guard Break Attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Enemy_Attack_Grab, "Ability.Cooldown.Enemy.Attack.Grab", "Cooldown for Grab Attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Enemy_Attack_Range, "Ability.Cooldown.Enemy.Attack.Range", "Cooldown for Range Attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Enemy_Attack_AoE, "Ability.Cooldown.Enemy.Attack.AoE", "Cooldown for AoE Attack");
}