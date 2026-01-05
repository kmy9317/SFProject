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
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Movement_Step);

	// Guard Break Attack (가드 파괴)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Attack_GuardBreak);

	// Grab Attack (잡기)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Attack_Grab);

	// Range Attack (원거리)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Attack_Range);

	// AoE Attack (범위)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Attack_AoE);

	// ========== Enemy State Ability Tags ==========

	// Turn In Place (제자리 회전)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_TurnInPlace);

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


	// ========== Hero Ability Tags ==========
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Interact);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Interact_Active);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Interact_Object);

	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Hero_Drink);

	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Hero_Dodge);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Hero_Downed);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Hero_Death);
	
	// Hero Skill Cooldowns
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Hero_Skill_Primary);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Hero_Skill_Secondary);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Hero_Skill_Identity);

	//Dragon
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dragon_Movement_Takeoff);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dragon_Movement_Land);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dragon_Movement_Swoop);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dragon_Movement_Hover);
	
	
}
