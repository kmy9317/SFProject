#pragma once

#include "CoreMinimal.h"
#include "SFGA_Hero_Base.h"
#include "SFGA_Equipment_Base.generated.h"

struct FSFWeaponWarpSettings;
class USFEquipmentDefinition;
class USFEquipmentComponent;
class USFCameraMode;
class USFEquipmentInstance;
class ASFEquipmentBase;
/**
 * 어빌리티가 활성화되기 위해 필요한 장비 요구사항을 정의
 */
USTRUCT(BlueprintType)
struct FSFEquipmentInfo
{
	GENERATED_BODY()

	// 장비 타입(장착된 장비 인스턴스 리턴용)
	UPROPERTY(EditAnywhere, Category = "SF|Equipment")
	FGameplayTag TypeTag;

	// 장비 슬롯 태그(올바른 장비 슬롯 확인용)
	UPROPERTY(EditAnywhere, Category = "SF|Equipment")
	FGameplayTag SlotTag;
};

/**
 * 장비(무기, 방패 등)가 필요한 어빌리티의 베이스 클래스
 */
UCLASS()
class SF_API USFGA_Equipment_Base : public USFGA_Hero_Base
{
	GENERATED_BODY()

public:
	USFGA_Equipment_Base(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:

	// ========== 슬롯 기반 접근 =========
	
	// 장착된 무기 액터 가져오기 (특정 슬롯)
	UFUNCTION(BlueprintCallable, Category = "SF|Equipment")
	AActor* GetEquippedActorBySlot(const FGameplayTag& SlotTag) const;

	// 특정 슬롯의 EquipmentInstance
	UFUNCTION(BlueprintCallable, Category = "SF|Equipment")
	USFEquipmentInstance* GetEquipmentInstanceBySlot(const FGameplayTag& SlotTag) const;

	// 특정 슬롯의 EquipmentDefinition
	UFUNCTION(BlueprintCallable, Category = "SF|Equipment")
	USFEquipmentDefinition* GetEquipmentDefinitionBySlot(const FGameplayTag& SlotTag) const;

	// ========== MainHand 단축 접근 ==========
	
	// 주무기 액터 가져오기
	UFUNCTION(BlueprintCallable, Category = "SF|Equipment|MainHand")
	AActor* GetMainHandWeaponActor() const;

	UFUNCTION(BlueprintCallable, Category = "SF|Equipment|MainHand")
	USFEquipmentInstance* GetMainHandEquipmentInstance() const;

	UFUNCTION(BlueprintCallable, Category = "SF|Equipment|MainHand")
	USFEquipmentDefinition* GetMainHandEquipmentDefinition() const;

	// ========== Warp Settings 접근 ==========
	
	// MainHand 무기의 WarpSettings (없으면 nullptr)
	const FSFWeaponWarpSettings* GetMainHandWarpSettings() const;

	// 특정 슬롯의 WarpSettings
	const FSFWeaponWarpSettings* GetWarpSettingsBySlot(const FGameplayTag& SlotTag) const;
	
protected:

	// 어빌리티 활성화를 위한 필요한 장비 요구사항 목록
	UPROPERTY(EditDefaultsOnly, Category="SF|Equipment")
	TArray<FSFEquipmentInfo> EquipmentInfos;
	
	UPROPERTY(EditDefaultsOnly, Category="SF|Equipment")
	TSubclassOf<USFCameraMode> CameraModeClass;
};
