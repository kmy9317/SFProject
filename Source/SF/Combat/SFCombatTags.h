#pragma once

#include "NativeGameplayTags.h"

namespace SFGameplayTags
{
	// ========== Combat Phase Tags ==========
	// 선딜레이 - 방향 전환 가능, Warp Target 업데이트 중
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Phase_Windup);
    
	// 활성 - 판정 구간, 방향 고정, Warp Target 확정
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Phase_Active);
    
	// 후딜레이 - 입력 버퍼 활성, 캔슬 가능 구간
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Phase_Recovery);

	// ========== Combat Phase Transition Events ==========
	// Windup → Active 전환 시점 (방향 확정 트리거)
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Event_Commit);
    
	// Active → Recovery 전환 시점
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Event_ReleaseComplete);
}