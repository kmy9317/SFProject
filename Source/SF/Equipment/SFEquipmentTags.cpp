// SFEquipmentTags.cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "SFEquipmentTags.h"


namespace SFGameplayTags
{
	// ========== 장비 타입 (Equipment Type) ==========
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentTag_Weapon, "Equipment.Weapon", "Weapon Equipment Tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentTag_Weapon_Melee, "Equipment.Weapon.Melee", "Melee Weapon Equipment Tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentTag_Weapon_Shield, "Equipment.Weapon.Shield", "Shield Weapon Equipment Tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentTag_Weapon_Ranged, "Equipment.Weapon.Ranged", "Ranged Weapon Equipment Tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentTag_Armor, "Equipment.Armor", "Armor Equipment Tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentTag_Accessory, "Equipment.Accessory", "Accessory Equipment Tag");

	// ========== 장비 슬롯 (Equipment Slot) ==========
	// 무기 슬롯
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentSlot_MainHand, "Equipment.Slot.MainHand", "Main Hand Weapon Slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentSlot_OffHand, "Equipment.Slot.OffHand", "Off Hand Weapon Slot");

	// 방어구 슬롯
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentSlot_Head, "Equipment.Slot.Head", "Head Armor Slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentSlot_Chest, "Equipment.Slot.Chest", "Chest Armor Slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentSlot_Hands, "Equipment.Slot.Hands", "Hands Armor Slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentSlot_Legs, "Equipment.Slot.Legs", "Legs Armor Slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentSlot_Feet, "Equipment.Slot.Feet", "Feet Armor Slot");

	// 액세서리 슬롯
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentSlot_Ring1, "Equipment.Slot.Ring1", "Ring Slot 1");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentSlot_Ring2, "Equipment.Slot.Ring2", "Ring Slot 2");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipmentSlot_Necklace, "Equipment.Slot.Necklace", "Necklace Slot");
}