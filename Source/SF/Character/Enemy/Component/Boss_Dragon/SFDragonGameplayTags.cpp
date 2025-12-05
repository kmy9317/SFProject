// Fill out your copyright notice in the Description page of Project Settings.

#include "SFDragonGameplayTags.h"

namespace SFGameplayTags
{
	// Dragon Movement State Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_Grounded, "Dragon.Movement.Grounded", "Dragon is on the ground, walking or idle");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_TakingOff, "Dragon.Movement.TakingOff", "Dragon is taking off from ground");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_Flying, "Dragon.Movement.Flying", "Dragon is flying freely, tracking target");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_Hovering, "Dragon.Movement.Hovering", "Dragon is hovering in place");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_Diving, "Dragon.Movement.Diving", "Dragon is diving down rapidly");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_Gliding, "Dragon.Movement.Gliding", "Dragon is gliding smoothly");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_Landing, "Dragon.Movement.Landing", "Dragon is landing on ground");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dragon_Movement_Disabled, "Dragon.Movement.Disabled", "Dragon movement is disabled");
}
