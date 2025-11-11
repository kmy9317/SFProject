// Fill out your copyright notice in the Description page of Project Settings.


#include "SFCharacterGameplayTags.h"


namespace SFGameplayTags
{
	// ========== 생명 상태 (Life States) ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Dead, "Character.State.Dead", "Character is dead");
    
	// ========== 전투 상태 (Combat States) ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Attacking, "Character.State.Attacking", "Character is attacking");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Hit, "Character.State.Hit", "Character is hit/damaged");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Stunned, "Character.State.Stunned", "Character is stunned");
    
	// ========== 방어/회피 (Defense/Evasion) ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Blocking, "Character.State.Blocking", "Character is blocking");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Dodging, "Character.State.Dodging", "Character is dodging");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Parrying, "Character.State.Parrying", "Character is parrying");
    
	// ========== 경직/넉백 (Stagger/Knockback) ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Staggered, "Character.State.Staggered", "Character is staggered ");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Knockdown, "Character.State.Knockdown", "Character is knocked down");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Launched, "Character.State.Launched", "Character is launched into air");
}