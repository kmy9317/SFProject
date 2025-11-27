// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"


namespace SFGameplayTags
{
	// Combat Events
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_TraceHit);


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
	
}