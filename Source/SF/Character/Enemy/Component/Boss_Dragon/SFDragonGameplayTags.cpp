// Fill out your copyright notice in the Description page of Project Settings.

#include "SFDragonGameplayTags.h"

namespace SFGameplayTags
{
	// ========== Dragon Movement State Tags ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_Grounded, "Dragon.Movement.Grounded", "Dragon is on the ground, walking or idle");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_TakingOff, "Dragon.Movement.TakingOff", "Dragon is taking off from ground");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_Flying, "Dragon.Movement.Flying", "Dragon is flying freely, tracking target");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_Hovering, "Dragon.Movement.Hovering", "Dragon is hovering in place");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_Diving, "Dragon.Movement.Diving", "Dragon is diving down rapidly");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_Gliding, "Dragon.Movement.Gliding", "Dragon is gliding smoothly");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_Landing, "Dragon.Movement.Landing", "Dragon is landing on ground");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_Disabled, "Dragon.Movement.Disabled", "Dragon movement is disabled");

	// ========== Dragon Ability Tags ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dragon_Bite, "Ability.Dragon.Bite", "Dragon Bite Attack - Grab and hold target");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dragon_FlameBreath_Line, "Ability.Dragon.FlameBreath.Line", "Dragon Flame Breath Line Attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dragon_FlameBreath_Spin, "Ability.Dragon.FlameBreath.Spin", "Dragon Flame Breath Spin Attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dragon_Stomp, "Ability.Dragon.Stomp", "Dragon Stomp Attack - AoE shockwave");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dragon_TailSwipe, "Ability.Dragon.TailSwipe", "Dragon Tail Swipe Attack - Knockback");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dragon_DiveAttack, "Ability.Dragon.DiveAttack", "Dragon Dive Attack - Aerial attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dragon_Charge, "Ability.Dragon.Charge", "Dragon Charge Attach Target");

	// ========== Dragon Cooldown Tags ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Dragon_Bite, "Ability.Cooldown.Dragon.Bite", "Cooldown for Dragon Bite");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Dragon_FlameBreath_Line, "Ability.Cooldown.Dragon.FlameBreath.Line", "Cooldown for Dragon Flame Breath Line");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Dragon_FlameBreath_Spin, "Ability.Cooldown.Dragon.FlameBreath.Spin", "Cooldown for Dragon Flame Breath Spin");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Dragon_Stomp, "Ability.Cooldown.Dragon.Stomp", "Cooldown for Dragon Stomp");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Dragon_TailSwipe, "Ability.Cooldown.Dragon.TailSwipe", "Cooldown for Dragon Tail Swipe");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Dragon_DiveAttack, "Ability.Cooldown.Dragon.DiveAttack", "Cooldown for Dragon Dive Attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Cooldown_Dragon_Charge, "Ability.Cooldown.Dragon.Charge", "Cooldown for Dragon Charge");

	// ========== Dragon Pressure Tags ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Pressure_Forward, "Dragon.Pressure.Forward", "Player is pressured forward");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Pressure_Back, "Dragon.Pressure.Forward", "Player is pressured Back");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Pressure_All, "Dragon.Pressure.Forward", "Player is pressured All");
	
}
