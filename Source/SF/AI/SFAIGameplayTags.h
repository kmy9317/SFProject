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

	//AI 상태 판단 및 거리 체크용 태그
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_State_Combat);   // 전투 상태 (타겟 감지 중)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Range_Melee);    // 근접 공격 범위 내
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AI_Range_Guard);    // 경계/원거리 범위 내
}