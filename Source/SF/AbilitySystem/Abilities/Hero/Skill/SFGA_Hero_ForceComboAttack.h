#pragma once

#include "CoreMinimal.h"
#include "Paladin/SFGA_Thrust_DivineSlash.h"
#include "GameplayTagContainer.h"
#include "SFGA_Hero_ForceComboAttack.generated.h"

class USFGameplayAbility;

UCLASS()
class SF_API USFGA_Hero_ForceComboAttack : public USFGA_Thrust_DivineSlash
{
	GENERATED_BODY()

protected:
	//오버라이드
	virtual void CompleteCombo(UGameplayAbility* SourceAbility) override;

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	
protected:
	//복구될 GA
	UPROPERTY(EditDefaultsOnly, Category="SF|Combo|Replace")
	TSubclassOf<USFGameplayAbility> ReplaceAbilityClass;

	//교체 대상 슬롯(InputTag)
	UPROPERTY(EditDefaultsOnly, Category="SF|Combo|Replace")
	FGameplayTag ReplaceInputTag;
};
