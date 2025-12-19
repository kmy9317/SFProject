// Fill out your copyright notice in the Description page of Project Settings.

#include "SFCharacterGameplayTags.h"

namespace SFGameplayTags
{
	// ========== 생명 상태 ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Dead, "Character.State.Dead", "Character is dead");

	// ========== 전투 상태 ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Attacking, "Character.State.Attacking", "Character is attacking");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Hit, "Character.State.Hit", "Character is hit/damaged");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Stunned, "Character.State.Stunned", "Character is stunned");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_ComboWindow, "Character.State.ComboWindow", "Valid input window for next combo attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_UsingAbility, "Character.State.UsingAbility", "Character UseAbility");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Skill, "Character.State.Skill", "Character Using Skill");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Charging, "Character.State.Charging", "Character is currently charging an attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Downed, "Character.State.Downed", "Character is downed");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_TurningInPlace, "Character.State.TurningInPlace", "Character is turning in place");

	// ========== 방어/회피 ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Blocking, "Character.State.Blocking", "Character is blocking");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Dodging, "Character.State.Dodging", "Character is dodging");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Parrying, "Character.State.Parrying", "Character is parrying");

	// ========== 경직/넉백 ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Staggered, "Character.State.Staggered", "Character is staggered");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Knockdown, "Character.State.Knockdown", "Character is knocked down");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Knockback, "Character.State.Knockback", "Character is knocked back");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Launched, "Character.State.Launched", "Character is launched upward or outward");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Grabbed, "Character.State.Grabbed", "Character is Grabbed ");

	// ========== 패링/그로기 ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Parried, "Character.State.Parried", "Character is parried (staggered due to parry)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Groggy, "Character.State.Groggy", "Character is in groggy state");

	// ========== 이펙트 ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Invulnerable, "Character.State.Invulnerable", "Character is invulnerable");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_SuperArmor, "Character.State.SuperArmor", "Character has super armor effect");

	UE_DEFINE_GAMEPLAY_TAG(State_RegenPaused_Health, "State.RegenPaused.Health");
	UE_DEFINE_GAMEPLAY_TAG(State_RegenPaused_Mana, "State.RegenPaused.Mana");
	UE_DEFINE_GAMEPLAY_TAG(State_RegenPaused_Stamina, "State.RegenPaused.Stamina");

	// ========== 상호작용 ==========
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Interact, "Character.State.Interact");
	
}
