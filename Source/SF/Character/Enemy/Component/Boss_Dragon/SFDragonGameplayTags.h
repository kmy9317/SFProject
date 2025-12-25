// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

/**
 * Dragon Boss Gameplay Tags
 */
namespace SFGameplayTags
{
	// ========== Dragon Movement State Tags ==========
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_Grounded);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_TakingOff);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_Flying);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_Hovering);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_Diving);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_Gliding);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_Landing);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_Disabled);

	// ========== Dragon Ability Tags ==========
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dragon_Bite);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dragon_FlameBreath_Line);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dragon_FlameBreath_Spin);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dragon_Stomp);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dragon_TailSwipe);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dragon_DiveAttack);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dragon_Charge);

	// ========== Dragon Cooldown Tags ==========
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Dragon_Bite);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Dragon_FlameBreath_Line);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Dragon_FlameBreath_Spin);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Dragon_Stomp);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Dragon_TailSwipe);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Dragon_DiveAttack);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Dragon_Charge);

	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Pressure_Forward);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Pressure_Back);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Pressure_All);
}
