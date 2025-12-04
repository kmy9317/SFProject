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
}
