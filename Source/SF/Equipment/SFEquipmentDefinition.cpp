// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/SFEquipmentDefinition.h"

FPrimaryAssetId USFEquipmentDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(GetEquipmentAssetType(), GetFName());
}

