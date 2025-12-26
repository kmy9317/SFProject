// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGameplayEventTags.h"


namespace SFGameplayTags
{
	// Combat Events
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_TraceHit, "GameplayEvent.TraceHit");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Montage_Begin, "GameplayEvent.Montage.Begin");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Montage_End, "GameplayEvent.Montage.End");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Tracing, "GameplayEvent.Tracing");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_ResetTrace, "GameplayEvent.ResetTrace");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_HitReaction, "GameplayEvent.HitReaction");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Death, "GameplayEvent.Death");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Parry, "GameplayEvent.Parry");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Parried, "GameplayEvent.Parried");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Knockback, "GameplayEvent.Knockback");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Launched, "GameplayEvent.Launched");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Groggy, "GameplayEvent.Groggy");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Revived, "GameplayEvent.Revived");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Downed, "GameplayEvent.Downed");

	// Damage Events
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Damage_Critical, "GameplayEvent.Damage.Critical");
    
	// Data Tags (SetByCaller)
    
	// Damage Data
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage_BaseDamage, "Data.Damage.BaseDamage");
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage_Multiplier, "Data.Damage.Multiplier");
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage_AttackPowerScaling, "Data.Damage.AttackPowerScaling");
    
	// Healing Data
	UE_DEFINE_GAMEPLAY_TAG(Data_Healing_BaseHealing, "Data.Healing.BaseHealing");
	UE_DEFINE_GAMEPLAY_TAG(Data_Healing_Multiplier, "Data.Healing.Multiplier");

	//StaggerData
	UE_DEFINE_GAMEPLAY_TAG(Data_Stagger_BaseStagger, "Data.Stagger.BaseStagger");
	UE_DEFINE_GAMEPLAY_TAG(Data_Stagger_Multiplier, "Data.Stagger.Multiplier");
    
	// Duration Data
	UE_DEFINE_GAMEPLAY_TAG(Data_Duration_Stun, "Data.Duration.Stun");
	UE_DEFINE_GAMEPLAY_TAG(Data_Duration_Root, "Data.Duration.Root");
	UE_DEFINE_GAMEPLAY_TAG(Data_Duration_Slow, "Data.Duration.Slow");

	UE_DEFINE_GAMEPLAY_TAG(Data_EnemyAbility_Cooldown, "Data.EnemyAbility.Cooldown");
	// Enemy Ability Data - Attack
	UE_DEFINE_GAMEPLAY_TAG(Data_EnemyAbility_BaseDamage, "Data.EnemyAbility.BaseDamage");
	UE_DEFINE_GAMEPLAY_TAG(Data_EnemyAbility_AttackRange, "Data.EnemyAbility.AttackRange");
	UE_DEFINE_GAMEPLAY_TAG(Data_EnemyAbility_AttackAngle, "Data.EnemyAbility.AttackAngle");
	UE_DEFINE_GAMEPLAY_TAG(Data_EnemyAbility_MinAttackRange, "Data.EnemyAbility.MinAttackRange");
    
	// Enemy Ability Data - Buff
	UE_DEFINE_GAMEPLAY_TAG(Data_EnemyAbility_BuffValue, "Data.EnemyAbility.BuffValue");
	UE_DEFINE_GAMEPLAY_TAG(Data_EnemyAbility_Duration, "Data.EnemyAbility.Duration");

	//Attribute Data
	// 공용 속성
	UE_DEFINE_GAMEPLAY_TAG(Data_MaxHealth, "Data.Attribute.MaxHealth");
	UE_DEFINE_GAMEPLAY_TAG(Data_AttackPower, "Data.Attribute.AttackPower");
	UE_DEFINE_GAMEPLAY_TAG(Data_MoveSpeed, "Data.Attribute.MoveSpeed");
	UE_DEFINE_GAMEPLAY_TAG(Data_Defense, "Data.Attribute.Defense");
	UE_DEFINE_GAMEPLAY_TAG(Data_CriticalDamage, "Data.Attribute.CriticalDamage");
	UE_DEFINE_GAMEPLAY_TAG(Data_CriticalChance, "Data.Attribute.CriticalChance");

	// Enemy 전용 속성
	UE_DEFINE_GAMEPLAY_TAG(Data_Enemy_MaxStagger, "Data.Attribute.Enemy.MaxStagger");
	UE_DEFINE_GAMEPLAY_TAG(Data_Enemy_GuardRange, "Data.Attribute.Enemy.GuardRange");
	UE_DEFINE_GAMEPLAY_TAG(Data_Enemy_SightRadius, "Data.Attribute.Enemy.SightRadius");
	UE_DEFINE_GAMEPLAY_TAG(Data_Enemy_LoseSightRadius, "Data.Attribute.Enemy.LoseSightRadius");

	// Cost
	UE_DEFINE_GAMEPLAY_TAG(Data_Cost_DodgeStamina, "Data.Cost.DodgeStamina");

	// Stage
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Stage_Clear, "GameplayEvent.Stage.Clear");

	// Player Buff
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_PlayerAbility_LastStand, "GameplayEvent.PlayerAbility.LastStand");
}