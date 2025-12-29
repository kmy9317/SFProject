#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace SFGameplayTags
{
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Skill_HeartBreaker_Charging)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Skill_Buff)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Skill_Wave)
	
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_HitReaction);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_HitReaction_Type_Player);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_HitReaction_Type_Enemy);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_HitReaction_Type_Normal);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_HitReaction_Heavy)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_HitReaction_Light)
	
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Event_Parry);
	
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Animation_PlayMontage);
}