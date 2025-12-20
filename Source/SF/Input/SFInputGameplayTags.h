#pragma once

#include "NativeGameplayTags.h"

namespace SFGameplayTags
{
	// Native Input Tags
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Mouse);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Crouch);

	// Ability Input Tags
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_PrimarySkill);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_SecondarySkill);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_IdentitySkill);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Attack);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Dodge);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Interact);
}