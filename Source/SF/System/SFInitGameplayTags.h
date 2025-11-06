#pragma once

#include "NativeGameplayTags.h"

namespace SFGameplayTags
{
	/** Init State Tags */
	SF_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_Spawned);
	SF_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_DataAvailable);
	SF_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_DataInitialized);
	SF_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_GameplayReady);
}