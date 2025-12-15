#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Hero_Parrying.h"
#include "SFGA_Hero_Parrying_C.generated.h"

class USFGameplayAbility;

UCLASS()
class SF_API USFGA_Hero_Parrying_C : public USFGA_Hero_Parrying
{
	GENERATED_BODY()

protected:
	//=======================Parry=======================
	virtual void OnParryEventReceived(FGameplayEventData Payload) override;
	//===================================================

	//==================Replace Ability==================
	//바꿀 GA
	UPROPERTY(EditDefaultsOnly, Category="SF|Parry|Replace")
	TSubclassOf<USFGameplayAbility> ReplacementAbilityClass;

	//교체할 슬롯(InputTag)
	UPROPERTY(EditDefaultsOnly, Category="SF|Parry|Replace")
	FGameplayTag ReplacementInputTag;
	//===================================================
	
private:
	void ReplaceAbilityOnServer();
};
