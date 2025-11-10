// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"


namespace SFGameplayTags
{

	
	// Enemy State Tags
	// Normal Enemy
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Enemy_State_Normal_Base);
    
	// Elite Enemy
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Enemy_State_Elite_Base);

    
	// Boss Enemy
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Enemy_State_Boss_Phase1);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Enemy_State_Boss_Phase2);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Enemy_State_Boss_Phase3);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Enemy_State_Boss_Groggy);
	
	// Enemy Behaviour Tags
	// Normal Enemy
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Enemy_Behaviour_Normal_Default);
    
	// Elite Enemy
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Enemy_Behaviour_Elite_Default);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Enemy_Behaviour_Elite_Enraged);
    
	// Boss Enemy
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Enemy_Behaviour_Boss_Phase1);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Enemy_Behaviour_Boss_Phase2);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Enemy_Behaviour_Boss_Phase3);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Enemy_Behaviour_Boss_Groggy);
}