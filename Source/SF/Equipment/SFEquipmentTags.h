// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

// SFEquipmentTags.h
namespace SFGameplayTags
{

	// ========== 장비 타입 (Equipment Type) ==========
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentTag_Weapon);
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentTag_Weapon_Melee);      // 근접 무기
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentTag_Weapon_Shield);      // 방패 무기
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentTag_Weapon_Ranged);     // 원거리 무기
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentTag_Armor);             // 방어구
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentTag_Accessory);         // 액세서리

	// ========== 장비 슬롯 (Equipment Slot) ==========
	// 무기 슬롯
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentSlot_MainHand);         // 주무기 슬롯
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentSlot_OffHand);          // 보조무기 슬롯

	// 방어구 슬롯
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentSlot_Head);             // 투구 슬롯
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentSlot_Chest);            // 갑옷 슬롯
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentSlot_Hands);            // 장갑 슬롯
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentSlot_Legs);             // 다리 방어구 슬롯
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentSlot_Feet);             // 신발 슬롯

	// 액세서리 슬롯
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentSlot_Ring1);            // 반지 슬롯 1
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentSlot_Ring2);            // 반지 슬롯 2
	SF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipmentSlot_Necklace);         // 목걸이 슬롯
}