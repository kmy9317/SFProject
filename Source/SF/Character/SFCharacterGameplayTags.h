// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "NativeGameplayTags.h"


namespace SFGameplayTags
{
	// ========== 생명 상태 (Life States) ==========
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Dead);
    
	// ========== 전투 상태 (Combat States) ==========
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Attacking);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Hit);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Stunned);

	// ========== 방어/회피 (Defense/Evasion) ==========
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Blocking);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Dodging);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Parrying);

	// ========== 경직/넉백 (Stagger/Knockback) ==========
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Staggered);  // Enemy 전용 경직도 
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Knockdown);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Launched);   // 날아가는 거 ? 
	
	

}
