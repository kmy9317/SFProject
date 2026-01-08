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
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Movement_Step, "Ability.Enemy.Movement.Step", "Step Movement");
	

	// Guard Break Attack (가드 파괴)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Attack_GuardBreak, "Ability.Enemy.Attack.GuardBreak", "Guard Break Attack - Breaks player guard");

	// Grab Attack (잡기)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Attack_Grab, "Ability.Enemy.Attack.Grab", "Grab Attack - Unblockable grab");

	// Range Attack (원거리)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Attack_Range, "Ability.Enemy.Attack.Range", "Range Attack - Ranged attack");

	// AoE Attack (범위)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Attack_AoE, "Ability.Enemy.Attack.AoE", "AoE Attack - Area of Effect attack");

	// ========== Enemy State Ability Tags ==========

	// Turn In Place (제자리 회전)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_TurnInPlace, "Ability.Enemy.TurnInPlace", "Turn In Place - Character rotation in place");

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

	// ========== Hero Ability Tags ==========
	UE_DEFINE_GAMEPLAY_TAG(Ability_Interact, "Ability.Interact");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Interact_Active, "Ability.Interact.Active")
	UE_DEFINE_GAMEPLAY_TAG(Ability_Interact_Object, "Ability.Interact.Object")
	
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hero_Drink, "Ability.Hero.Drink")
	
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hero_Dodge, "Ability.Hero.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hero_Downed, "Ability.Hero.Downed");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hero_Death, "Ability.Hero.Death");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Sprint, "Ability.Sprint");
	
	// Hero Skill Cooldowns
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Hero_Skill_Primary, "Ability.Cooldown.Hero.Skill.Primary", "Cooldown for primary skill");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Hero_Skill_Secondary, "Ability.Cooldown.Hero.Skill.Secondary", "Cooldown for Secondary skill");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Hero_Skill_Identity, "Ability.Cooldown.Hero.Skill.Identity", "Cooldown for Identity skill");

	//Dragon
	UE_DEFINE_GAMEPLAY_TAG(Ability_Dragon_Movement_Takeoff, "Ability.Dragon.Movement.Takeoff");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Dragon_Movement_Land, "Ability.Dragon.Movement.Land");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Dragon_Movement_Swoop, "Ability.Dragon.Movement.Swoop");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Dragon_Movement_Hover, "Ability.Dragon.Movement.Hover");

	// Sorcerer Skill Tag

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Hero_Elemental_Fire, "Ability.Hero.Elemental.Fire", "Sorcerer Elemental Tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Hero_Elemental_Ice, "Ability.Hero.Elemental.Ice", "Sorcerer Elemental Tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Hero_Elemental_Electric, "Ability.Hero.Elemental.Electric", "Sorcerer Elemental Tag");
}