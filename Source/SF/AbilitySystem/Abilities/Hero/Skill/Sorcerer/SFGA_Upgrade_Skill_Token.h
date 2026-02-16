// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Upgrade_Skill_Token.generated.h"

/**
 * 스킬 강화용 토큰 어빌리티
 * 부여 즉시 관리자 GA에 오버라이드를 등록하고 자기 자신을 제거
 * SkillTypeChange의 CustomPersistentData에 내부의 강화 어빌리티는 저장됨
 */
UCLASS()
class SF_API USFGA_Upgrade_Skill_Token : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Upgrade_Skill_Token();

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	// 교체할 대상 속성 태그 (예: Element.Fire)
	UPROPERTY(EditDefaultsOnly, Category = "Upgrade")
	FGameplayTag TargetElementTag;

	// 교체할 대상 슬롯 태그 (예: InputTag.Ability.Primary)
	UPROPERTY(EditDefaultsOnly, Category = "Upgrade")
	FGameplayTag TargetInputTag;

	// 실제로 사용할 강화된 스킬 클래스 (예: GA_FireBall_Lv2)
	UPROPERTY(EditDefaultsOnly, Category = "Upgrade")
	TSubclassOf<USFGameplayAbility> NewAbilityClass;
};