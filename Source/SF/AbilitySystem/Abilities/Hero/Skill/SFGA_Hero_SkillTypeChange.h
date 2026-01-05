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

/**
 * Sorcerer 전용: 속성(Fire, Ice, Lightning)을 순환하며 스킬 셋을 교체 및 관리하는 어빌리티
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
	// [핵심] 외부(토큰 GA)에서 호출하여 스킬 교체 등록
	void RegisterSkillOverride(FGameplayTag ElementTag, FGameplayTag InputTag, TSubclassOf<USFGameplayAbility> NewAbilityClass);

protected:
	// 내부 로직: 다음 속성으로 순환
	void CycleToNextAbilitySet();

	// 내부 로직: 오버라이드를 적용하여 AbilitySet 부여 (SFAbilitySet::GiveToAbilitySystem 대체)
	void GiveAbilitySetWithOverrides(const USFAbilitySet* AbilitySet, FGameplayTag CurrentElementTag);

protected:
	/** * 순환할 어빌리티 세트 목록 (0: Fire, 1: Ice, 2: Lightning)
	 * 주의: 여기에 공격 스킬들이 포함된 DA를 넣어야 합니다. (자기 자신 제외)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SF|Sorcerer")
	TArray<TObjectPtr<USFAbilitySet>> AbilitySets;

	/**
	 * AbilitySets의 인덱스와 매칭되는 속성 태그
	 * 예: [0]=Element.Fire, [1]=Element.Ice, [2]=Element.Lightning
	 * 토큰 GA가 "Fire를 바꿔줘"라고 할 때 매칭하기 위해 필요합니다.
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

	// [저장소] 속성별 스킬 오버라이드 정보 (Key: ElementTag)
	UPROPERTY(Transient)
	TMap<FGameplayTag, FSFSkillOverrideInfo> ElementSkillOverrides;
};