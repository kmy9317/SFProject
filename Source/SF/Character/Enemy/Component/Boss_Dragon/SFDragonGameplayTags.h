// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

/**
 * Dragon Boss Movement State Tags
 */
namespace SFGameplayTags
{
	// Dragon Movement State Tags
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_Grounded);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_TakingOff);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_Flying);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_Hovering);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_Diving);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_Gliding);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_Landing);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dragon_Movement_Disabled);
}
