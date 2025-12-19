#pragma once
#include "NativeGameplayTags.h"

namespace SFGameplayTags
{
	// ========== 생명 상태 ==========
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Dead);

	// ========== 전투 상태 ==========
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Attacking);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Hit);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Stunned);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_ComboWindow);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_UsingAbility);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Skill);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Charging);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Downed);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_TurningInPlace);

	// ========== 방어/회피 ==========
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Blocking);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Dodging);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Parrying);

	// ========== 경직/넉백 ==========
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Staggered);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Knockdown);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Knockback);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Launched);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Parried);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Groggy);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Grabbed);
	

	// ========== 이펙트 ==========
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Invulnerable);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_SuperArmor);

	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Health);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Mana);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Stamina);
	
	// ========== 상호작용 ==========
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Interact);
}
