// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGameplayEventTags.h"

namespace SFGameplayTags
{
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_TraceHit, "GameplayEvent.TraceHit");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayEvent_Damage_Critical, "GameplayEvent.Damage.Critical");
	
	
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage_BaseDamage, "Data.Damage.BaseDamage");
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage_Multiplier, "Data.Damage.Multiplier");
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage_AttackPowerScaling, "Data.Damage.AttackPowerScaling");
    
	UE_DEFINE_GAMEPLAY_TAG(Data_Healing_BaseHealing, "Data.Healing.BaseHealing");
	UE_DEFINE_GAMEPLAY_TAG(Data_Healing_Multiplier, "Data.Healing.Multiplier");
    
	UE_DEFINE_GAMEPLAY_TAG(Data_Duration_Stun, "Data.Duration.Stun");
	UE_DEFINE_GAMEPLAY_TAG(Data_Duration_Root, "Data.Duration.Root");
	UE_DEFINE_GAMEPLAY_TAG(Data_Duration_Slow, "Data.Duration.Slow");
    
	
}