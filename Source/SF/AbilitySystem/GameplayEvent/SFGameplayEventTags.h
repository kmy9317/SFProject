// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"


namespace SFGameplayTags
{
	// Combat Events
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_TraceHit);

	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Montage_Begin);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Montage_End);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Tracing);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_ResetTrace);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_HitReaction);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Death);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Parry);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Parried);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Knockback);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Launched);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Groggy);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_MoveStep);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Revived);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Downed);

	// Turn In Place Events
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Turn_90L);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Turn_90R);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Turn_180L);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Turn_180R);

	//Damage Events
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Damage_Critical);
    
    
	// Data Tags (SetByCaller)

	// Damage Data
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage_BaseDamage);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage_Multiplier);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage_AttackPowerScaling);
    
	// Healing Data
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Healing_BaseHealing);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Healing_Multiplier);

	//Stagger Data
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Stagger_BaseStagger);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Stagger_Multiplier);
    
	// Duration Data
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Duration_Stun);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Duration_Root);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Duration_Slow);
    
	// Enemy Ability Data - Attack
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_EnemyAbility_Cooldown);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_EnemyAbility_BaseDamage);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_EnemyAbility_AttackRange);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_EnemyAbility_AttackAngle);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_EnemyAbility_MinAttackRange);
    
	// Enemy Ability Data - Buff
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_EnemyAbility_BuffValue);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_EnemyAbility_Duration);

	// Attribute Data
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Health);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_MaxHealth);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_AttackPower);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_MoveSpeed);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Defense);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_CriticalDamage);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_CriticalChance);

	// --- Enemy Attribute Data ---
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Enemy_MaxStagger);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Enemy_GuardRange);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Enemy_SightRadius);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Enemy_LoseSightRadius);

	// --- Hero Attribute Data ---
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Hero_Mana);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Hero_Stamina);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Hero_ManaRegen);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Hero_StaminaRegen);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Hero_CooldownReduction);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Hero_Luck);

	// Cooldown
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Cooldown_Base);
	
	// Cost
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Cost_Stamina);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Cost_DodgeStamina);

	// Stage
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Stage_Clear);

	// Player Buff
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_PlayerAbility_LastStand);

	//Dragon
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Dragon_Flight_Launch);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Dragon_Flight_Land);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Dragon_Flight_Swoop);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Dragon_Flight_Hover);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Dragon_Fireball_Launch);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Dragon_TakeOff_Completed);
	
	
}