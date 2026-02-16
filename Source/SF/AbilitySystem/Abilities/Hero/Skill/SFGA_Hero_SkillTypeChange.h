#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "AbilitySystem/SFAbilitySet.h"
#include "SFGA_Hero_SkillTypeChange.generated.h"

/**
 * 스킬 오버라이드 정보를 저장하는 구조체
 */
USTRUCT(BlueprintType)
struct FSFSkillOverrideInfo
{
	GENERATED_BODY()

	// Key: InputTag (예: InputTag.Ability.Primary)
	// Value: 교체할 어빌리티 클래스 (예: GA_FireBall_Lv2)
	UPROPERTY()
	TMap<FGameplayTag, TSubclassOf<USFGameplayAbility>> SlotOverrides;
};

USTRUCT(BlueprintType)
struct FSFSkillTypeChangeData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 CurrentSetIndex = 0;

	UPROPERTY()
	TMap<FGameplayTag, FSFSkillOverrideInfo> ElementOverrides;

	// 슬롯별 공유 레벨
	UPROPERTY()
	TMap<FGameplayTag, int32> SharedSlotLevels;
};

/**
 * Sorcerer 전용: 속성(Fire, Ice, Lightning)을 순환하며 스킬 셋을 교체 및 관리하는 어빌리티
 * CustomPersistentData를 통해 오버라이드 정보와 공유 레벨 스냅샷
 * OnAvatarSet에서 복원된 데이터를 기반으로 어빌리티 세트를 재부여
 */
UCLASS()
class SF_API USFGA_Hero_SkillTypeChange : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Hero_SkillTypeChange();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
protected:
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

public:

	// 커스텀 데이터 저장/복원 (FInstancedStruct 방식)
	virtual bool HasCustomPersistentData() const override { return true; }
	virtual FInstancedStruct SaveCustomPersistentData() const override;
	virtual void RestoreCustomPersistentData(const FInstancedStruct& InData) override;
	
	// [핵심] 외부(토큰 GA)에서 호출하여 스킬 교체 등록
	void RegisterSkillOverride(FGameplayTag ElementTag, FGameplayTag InputTag, TSubclassOf<USFGameplayAbility> NewAbilityClass);

protected:
	// 내부 로직: 다음 속성으로 순환
	void CycleToNextAbilitySet();

	// 현재 인덱스 기준으로 어빌리티 세트 부여
	void ApplyCurrentAbilitySet(USFAbilitySystemComponent* SFASC);

	// 내부 로직: 오버라이드를 적용하여 AbilitySet 부여 (SFAbilitySet::GiveToAbilitySystem 대체)
	void GiveAbilitySetWithOverrides(const USFAbilitySet* AbilitySet, FGameplayTag CurrentElementTag);

	// 현재 활성화된 슬롯들의 레벨을 ElementSkillOverrides에 캐싱
	void SyncCurrentSlotLevels();

	void SwapManagedAbility(USFAbilitySystemComponent* SFASC, FGameplayTag InputTag, TSubclassOf<USFGameplayAbility> NewAbilityClass);

protected:
	/** * 순환할 어빌리티 세트 목록 (0: Fire, 1: Ice, 2: Lightning)
	 * 여기에 공격 스킬들이 포함된 DA들 관리.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SF|Sorcerer")
	TArray<TObjectPtr<USFAbilitySet>> AbilitySets;

	/**
	 * AbilitySets의 인덱스와 매칭되는 속성 태그
	 * 예: [0]=Element.Fire, [1]=Element.Ice, [2]=Element.Lightning
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SF|Sorcerer")
	TArray<FGameplayTag> ElementTags;

	// 현재 활성화된 세트 인덱스
	int32 CurrentSetIndex;

	// 현재 부여된 핸들 관리 (회수용)
	UPROPERTY(Transient)
	FSFAbilitySet_GrantedHandles ActiveGrantedHandles;

	// 최초 실행 여부
	bool bHasActivatedOnce;

	// 복원된 데이터가 있는지 (OnAvatarSet에서 재초기화 방지용)
	bool bHasRestoredData;

	// [저장소] 속성별 스킬 오버라이드 정보 (Key: ElementTag)
	UPROPERTY(Transient)
	TMap<FGameplayTag, FSFSkillOverrideInfo> ElementSkillOverrides;

	// 슬롯별 공유 레벨 (모든 속성이 공유)
	UPROPERTY()
	TMap<FGameplayTag, int32> SharedSlotLevels;
};