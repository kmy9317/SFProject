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
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_MaxHealth);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_AttackPower);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_MoveSpeed);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Defense);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_CriticalDamage);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_CriticalChance);

	// --- Enemy Attribute Data ---
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Enemy_MaxStagger);
}