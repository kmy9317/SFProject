#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Equipment_Base.generated.h"

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
 * 
 */
UCLASS()
class SF_API USFGA_Equipment_Base : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Equipment_Base(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 장착된 무기 액터 가져오기 (특정 슬롯)
	UFUNCTION(BlueprintCallable, Category = "SF|Equipment")
	AActor* GetEquippedActorBySlot(const FGameplayTag& SlotTag) const;

	// 주무기 액터 가져오기
	UFUNCTION(BlueprintCallable, Category = "SF|Equipment")
	AActor* GetMainHandWeaponActor() const;

	// EquipmentComponent 가져오기
	USFEquipmentComponent* GetEquipmentComponent() const;
	
protected:

	// 어빌리티 활성화를 위한 필요한 장비 요구사항 목록
	UPROPERTY(EditDefaultsOnly, Category="SF|Equipment")
	TArray<FSFEquipmentInfo> EquipmentInfos;
	
	UPROPERTY(EditDefaultsOnly, Category="SF|Equipment")
	TSubclassOf<USFCameraMode> CameraModeClass;
};
