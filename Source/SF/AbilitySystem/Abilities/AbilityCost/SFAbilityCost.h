#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "UObject/Object.h"
#include "SFAbilityCost.generated.h"

class USFGameplayAbility;

UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class SF_API USFAbilityCost : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 비용 지불 가능 여부 확인 (클라이언트 + 서버)
	 * @return true면 Ability 활성화 가능
	 */
	virtual bool CheckCost(const USFGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
	{
		return true;
	}

	/**
	 * 비용 적용 (서버에서만)
	 */
	virtual void ApplyCost(const USFGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
	{
	}

	bool ShouldOnlyApplyCostOnHit() const { return bOnlyApplyCostOnHit; }

protected:
	/** true면 성공적으로 Hit했을 때만 비용 적용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
	bool bOnlyApplyCostOnHit = false;
};
