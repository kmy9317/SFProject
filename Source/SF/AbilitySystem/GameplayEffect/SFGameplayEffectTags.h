// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * 
 */
namespace SFGameplayTags
{
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Stunned);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_HitReaction);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Fail_NoStamina); // UI 피드백용
	
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Persist_OnTravel);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_RegenStamina_Block);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_RegenMana_Block);
}