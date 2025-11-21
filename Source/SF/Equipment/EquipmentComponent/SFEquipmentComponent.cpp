// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/SFCharacterBase.h"
#include "Equipment/SFEquipmentDefinition.h"
#include "Equipment/SFEquipmentTags.h"
#include "Equipment/EquipmentInstance/SFEquipmentInstance.h"




void USFEquipmentComponent::EquipItem(USFEquipmentDefinition* EquipmentDefinition)
{
	if (!EquipmentDefinition)
	{
		return;
	}

	ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(GetOwner());
	if (!SFCharacter)
	{
		return;
	}
    
	USFAbilitySystemComponent* ASC = SFCharacter->GetSFAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	if (EquipmentDefinition->EquipmentSlotTag.IsValid())
	{
		// 같은 슬롯에 장착된 장비가 있다면 제거
		if (USFEquipmentInstance* ExistingInstance = FindEquipmentInstanceBySlot(EquipmentDefinition->EquipmentSlotTag))
		{
			UnequipItemByInstance(ExistingInstance);
		}
	}

	USFEquipmentInstance* NewInstance = NewObject<USFEquipmentInstance>(this);
	NewInstance->Initialize(EquipmentDefinition, SFCharacter, ASC);
	EquipmentInstances.Add(NewInstance);
}

void USFEquipmentComponent::UnequipItem(FGameplayTag EquipmentSlotTag)
{
	// 슬롯 기반으로 장비 제거
	if (USFEquipmentInstance* EquipmentInstance = FindEquipmentInstanceBySlot(EquipmentSlotTag))
	{
		UnequipItemByInstance(EquipmentInstance);
	}
}

void USFEquipmentComponent::InitializeEquipment()
{
	for (USFEquipmentDefinition* EquipmentDefinition : DefaultEquipmentDefinitions)
	{
		EquipItem(EquipmentDefinition);
	}
}

void USFEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeEquipment();
}

USFEquipmentInstance* USFEquipmentComponent::FindEquipmentInstance(FGameplayTag EquipmentTag) const
{
	for (USFEquipmentInstance* EquipmentInstance : EquipmentInstances)
	{
		if (EquipmentInstance &&
			EquipmentInstance->GetEquipmentDefinition() &&
			EquipmentInstance->GetEquipmentDefinition()->EquipmentTag.MatchesTag(EquipmentTag))
		{
			return EquipmentInstance;
		}
	}
	return nullptr;
}

USFEquipmentInstance* USFEquipmentComponent::FindEquipmentInstanceBySlot(FGameplayTag SlotTag) const
{
	for (USFEquipmentInstance* EquipmentInstance : EquipmentInstances)
	{
		if (EquipmentInstance &&
			EquipmentInstance->GetEquipmentDefinition() &&
			EquipmentInstance->GetEquipmentDefinition()->EquipmentSlotTag == SlotTag)
		{
			return EquipmentInstance;
		}
	}
	return nullptr;
}

void USFEquipmentComponent::UnequipItemByInstance(USFEquipmentInstance* EquipmentInstance)
{
	if (!EquipmentInstance)
	{
		return;
	}

	ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(GetOwner());
	if (SFCharacter)
	{
		USFAbilitySystemComponent* ASC = SFCharacter->GetSFAbilitySystemComponent();
		EquipmentInstance->Deinitialize(ASC);
	}

	EquipmentInstances.Remove(EquipmentInstance);
}