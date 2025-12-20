#include "SFGA_Equipment_Base.h"

#include "Character/SFCharacterBase.h"
#include "Equipment/SFEquipmentTags.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "Equipment/EquipmentInstance/SFEquipmentInstance.h"

USFGA_Equipment_Base::USFGA_Equipment_Base(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bServerRespectsRemoteAbilityCancellation = false;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnlyTermination;
}

bool USFGA_Equipment_Base::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(ActorInfo->AvatarActor.Get());
	if (!SFCharacter)
	{
		return false;
	}

	USFEquipmentComponent* EquipmentComponent = SFCharacter->FindComponentByClass<USFEquipmentComponent>();
	if (!EquipmentComponent)
	{
		return false;
	}

	// EquipmentInfos 배열에 정의된 모든 장비 요구사항 검증
	for (const FSFEquipmentInfo& EquipmentInfo : EquipmentInfos)
	{
		if (EquipmentInfo.TypeTag == FGameplayTag::EmptyTag)
		{
			return false;
		}

		// 무기 타입 체크
		if (EquipmentInfo.TypeTag.MatchesTag(SFGameplayTags::EquipmentTag_Weapon))
		{
			// 장착된 무기 인스턴스 가져오기
			USFEquipmentInstance* EquipmentInstance = EquipmentComponent->FindEquipmentInstance(EquipmentInfo.TypeTag);
			if (!EquipmentInstance)
			{
				return false;
			}

			// 무기 인스턴스의 정보를 담고있는 Definition 가져오기
			USFEquipmentDefinition* EquipmentDefinition = EquipmentInstance->GetEquipmentDefinition();
			if (!EquipmentDefinition)
			{
				return false;
			}
			// 슬롯 확인(현재 무기가 장착된 위치와 일치하는지 확인)
			if (!EquipmentDefinition->EquipmentSlotTag.MatchesTagExact(EquipmentInfo.SlotTag))
			{
				return false;
			}
		}
	}

	return true;
}

void USFGA_Equipment_Base::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

AActor* USFGA_Equipment_Base::GetEquippedActorBySlot(const FGameplayTag& SlotTag) const
{
	if (USFEquipmentComponent* EquipComp = GetEquipmentComponent())
	{
		return EquipComp->GetFirstEquippedActorBySlot(SlotTag);
	}
	return nullptr;
}

USFEquipmentInstance* USFGA_Equipment_Base::GetEquipmentInstanceBySlot(const FGameplayTag& SlotTag) const
{
	if (USFEquipmentComponent* EquipComp = GetEquipmentComponent())
	{
		return EquipComp->FindEquipmentInstanceBySlot(SlotTag);
	}
	return nullptr;
}

USFEquipmentDefinition* USFGA_Equipment_Base::GetEquipmentDefinitionBySlot(const FGameplayTag& SlotTag) const
{
	if (USFEquipmentInstance* Instance = GetEquipmentInstanceBySlot(SlotTag))
	{
		return Instance->GetEquipmentDefinition();
	}
	return nullptr;
}

AActor* USFGA_Equipment_Base::GetMainHandWeaponActor() const
{
	return GetEquippedActorBySlot(SFGameplayTags::EquipmentSlot_MainHand);
}

USFEquipmentInstance* USFGA_Equipment_Base::GetMainHandEquipmentInstance() const
{
	return GetEquipmentInstanceBySlot(SFGameplayTags::EquipmentSlot_MainHand);
}

USFEquipmentDefinition* USFGA_Equipment_Base::GetMainHandEquipmentDefinition() const
{
	return GetEquipmentDefinitionBySlot(SFGameplayTags::EquipmentSlot_MainHand);
}

const FSFWeaponWarpSettings* USFGA_Equipment_Base::GetMainHandWarpSettings() const
{
	return GetWarpSettingsBySlot(SFGameplayTags::EquipmentSlot_MainHand);
}

const FSFWeaponWarpSettings* USFGA_Equipment_Base::GetWarpSettingsBySlot(const FGameplayTag& SlotTag) const
{
	if (USFEquipmentDefinition* Definition = GetEquipmentDefinitionBySlot(SlotTag))
	{
		return &Definition->WarpSettings;
	}
	return nullptr;
}

void USFGA_Equipment_Base::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
}

