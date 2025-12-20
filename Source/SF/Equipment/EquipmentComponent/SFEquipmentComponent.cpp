// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/SFCharacterBase.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Equipment/SFEquipmentDefinition.h"
#include "Equipment/SFEquipmentTags.h"
#include "Equipment/EquipmentInstance/SFEquipmentInstance.h"
#include "Net/UnrealNetwork.h"
#include "Weapons/Actor/SFEquipmentBase.h"
#include "Weapons/Actor/SFMeleeWeaponActor.h"


USFEquipmentComponent::USFEquipmentComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, EquipmentList(this)
	, bEquipmentInitialized(false)
{
	PrimaryComponentTick.bCanEverTick = false;
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicatedByDefault(true);
}

void USFEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, EquipmentList);
	DOREPLIFETIME(ThisClass, bShouldHiddenWeaponActors);
}

void USFEquipmentComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	// 이미 존재하는 Instance들 등록
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FSFAppliedEquipmentEntry& Entry : EquipmentList.Entries)
		{
			if (Entry.Instance)
			{
				AddReplicatedSubObject(Entry.Instance);
			}
		}
	}
}

void USFEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	if (USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		PawnExtComp->OnAbilitySystemInitialized_RegisterAndCall(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
        
		PawnExtComp->OnAbilitySystemUninitialized_Register(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));
	}
	else
	{
		if (GetAbilitySystemComponent())
		{
			InitializeEquipment();
		}
	}
}

void USFEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UninitializeAllEquipment();
	
	Super::EndPlay(EndPlayReason);
}

void USFEquipmentComponent::OnAbilitySystemInitialized()
{
	InitializeEquipment();
}

void USFEquipmentComponent::OnAbilitySystemUninitialized()
{
	UninitializeAllEquipment();
}

USFAbilitySystemComponent* USFEquipmentComponent::GetAbilitySystemComponent() const
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return nullptr;
	}

	if (USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		return PawnExtComp->GetSFAbilitySystemComponent();
	}

	if (ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(Pawn))
	{
		return SFCharacter->GetSFAbilitySystemComponent();
	}

	return nullptr;
}

void USFEquipmentComponent::EquipItem(USFEquipmentDefinition* EquipmentDefinition)
{
	if (!EquipmentDefinition)
	{
		return;
	}

	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn || !Pawn->HasAuthority())
	{
		return;
	}
    
	USFAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (EquipmentDefinition->EquipmentSlotTag.IsValid())
	{
		// 같은 슬롯에 장착된 장비가 있다면 제거
		if (USFEquipmentInstance* ExistingInstance = FindEquipmentInstanceBySlot(EquipmentDefinition->EquipmentSlotTag))
		{
			UnequipItemByInstance(ExistingInstance);
		}
	}

	// FastArray에 Entry 추가
	FSFAppliedEquipmentEntry& NewEntry = EquipmentList.Entries.AddDefaulted_GetRef();
	NewEntry.Instance = NewObject<USFEquipmentInstance>(this);
	NewEntry.Instance->Initialize(EquipmentDefinition, Pawn, ASC);

	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
	{
		AddReplicatedSubObject(NewEntry.Instance);
	}
	
	EquipmentList.MarkItemDirty(NewEntry);
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
	if (bEquipmentInitialized)
	{
		return;
	}

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	if (Pawn->HasAuthority())
	{
		for (USFEquipmentDefinition* EquipmentDef : DefaultEquipmentDefinitions)
		{
			if (EquipmentDef)
			{
				EquipItem(EquipmentDef);
			}
		}
	}

	bEquipmentInitialized = true;
}

void USFEquipmentComponent::UninitializeAllEquipment()
{
	if (!bEquipmentInitialized)
	{
		return;
	}

	APawn* Pawn = GetPawn<APawn>();
	if (Pawn && Pawn->HasAuthority())
	{
		TArray<USFEquipmentInstance*> AllInstances = GetEquippedItems();
		for (int32 i = AllInstances.Num() - 1; i >= 0; --i)
		{
			UnequipItemByInstance(AllInstances[i]);
		}
	}

	bEquipmentInitialized = false;
}

USFEquipmentInstance* USFEquipmentComponent::FindEquipmentInstance(FGameplayTag EquipmentTag) const
{
	for (const FSFAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.Instance &&
			Entry.Instance->GetEquipmentDefinition() &&
			Entry.Instance->GetEquipmentDefinition()->EquipmentTag.MatchesTag(EquipmentTag))
		{
			return Entry.Instance;
		}
	}
	return nullptr;
}

USFEquipmentInstance* USFEquipmentComponent::FindEquipmentInstanceBySlot(FGameplayTag SlotTag) const
{
	for (const FSFAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.Instance &&
			Entry.Instance->GetEquipmentDefinition() &&
			Entry.Instance->GetEquipmentDefinition()->EquipmentSlotTag == SlotTag)
		{
			return Entry.Instance;
		}
	}
	return nullptr;
}

void USFEquipmentComponent::GetAllEquippedActors(TArray<AActor*>& OutActors) const
{
	OutActors.Reset();

	const TArray<FSFAppliedEquipmentEntry>& CurrentEquipmentListEntry = EquipmentList.Entries;
	for (const FSFAppliedEquipmentEntry& Entry : CurrentEquipmentListEntry)
	{
		if (Entry.Instance)
		{
			OutActors.Append(Entry.Instance->GetSpawnedActors());
		}
	}
}

void USFEquipmentComponent::GetEquippedWeaponActors(TArray<AActor*>& OutActors) const
{
	OutActors.Reset();

	const TArray<FSFAppliedEquipmentEntry>& CurrentEquipmentListEntry = EquipmentList.Entries;
	for (const FSFAppliedEquipmentEntry& Entry : CurrentEquipmentListEntry)
	{
		if (Entry.Instance && Entry.Instance->GetEquipmentDefinition())
		{
			// EquipmentTag가 Weapon 태그와 매칭되는지 확인
			if (Entry.Instance->GetEquipmentDefinition()->EquipmentTag.MatchesTag(SFGameplayTags::EquipmentTag_Weapon))
			{
				OutActors.Append(Entry.Instance->GetSpawnedActors());
			}
		}
	}
}

void USFEquipmentComponent::UnequipItemByInstance(USFEquipmentInstance* EquipmentInstance)
{
	if (!EquipmentInstance)
	{
		return;
	}

	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn || !Pawn->HasAuthority())
	{
		return;
	}

	USFAbilitySystemComponent* ASC = GetAbilitySystemComponent();

	for (auto EntryIt = EquipmentList.Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FSFAppliedEquipmentEntry& Entry = *EntryIt;
		if (Entry.Instance == EquipmentInstance)
		{
			// SubObject 등록 해제
			if (IsUsingRegisteredSubObjectList())
			{
				RemoveReplicatedSubObject(Entry.Instance);
			}
			
			Entry.Instance->Deinitialize(ASC);
			
			EntryIt.RemoveCurrent();
			EquipmentList.MarkArrayDirty();
			return;
		}
	}
}

TArray<USFEquipmentInstance*> USFEquipmentComponent::GetEquippedItems() const
{
	TArray<USFEquipmentInstance*> Results;
	for (const FSFAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.Instance)
		{
			Results.Add(Entry.Instance);
		}
	}
	return Results;
}

AActor* USFEquipmentComponent::GetFirstEquippedActorBySlot(const FGameplayTag& SlotTag) const
{
	if (USFEquipmentInstance* Instance = FindEquipmentInstanceBySlot(SlotTag))
	{
		TArray<AActor*> SpawnedActors = Instance->GetSpawnedActors();
		if (SpawnedActors.Num() > 0)
		{
			return SpawnedActors[0];
		}
	}
	return nullptr;
}

bool USFEquipmentComponent::IsSlotEquipmentMatchesTag(const FGameplayTag& SlotTag, const FGameplayTag& CheckingTag) const
{
	if (USFEquipmentInstance* Instance = FindEquipmentInstanceBySlot(SlotTag))
	{
		if (Instance->GetEquipmentDefinition())
		{
			return Instance->GetEquipmentDefinition()->EquipmentTag.MatchesTag(CheckingTag);
		}
	}
	return false;
}

FGameplayTag USFEquipmentComponent::GetMainHandEquipMontageTag() const
{
	AActor* EquippedActor = GetFirstEquippedActorBySlot(SFGameplayTags::EquipmentSlot_MainHand);
	if (ASFEquipmentBase* Equipment = Cast<ASFEquipmentBase>(EquippedActor))
	{
		return Equipment->GetEquipMontageTag();
	}
	return FGameplayTag();
}

void USFEquipmentComponent::ShowWeapons()
{
	ChangeShouldHiddenWeaponActors(false);
}

void USFEquipmentComponent::HideWeapons()
{
	ChangeShouldHiddenWeaponActors(true);
}

void USFEquipmentComponent::ChangeShouldHiddenWeaponActors(bool bNewShouldHiddenEquipments)
{
	bShouldHiddenWeaponActors = bNewShouldHiddenEquipments;

	TArray<AActor*> OutWeaponActors;
	GetEquippedWeaponActors(OutWeaponActors);

	for (AActor* WeaponActor : OutWeaponActors)
	{
		if (ASFEquipmentBase* Weapon = Cast<ASFEquipmentBase>(WeaponActor))
		{
			Weapon->SetActorHiddenInGame(bShouldHiddenWeaponActors);
		}
	}
}

