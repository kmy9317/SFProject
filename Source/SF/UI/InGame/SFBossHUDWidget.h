#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFBossHUDWidget.generated.h"

class UAbilitySystemComponent;
class ASFEnemy;
struct FOnAttributeChangeData;
/**
 * 
 */



UCLASS()
class SF_API USFBossHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SF|UI")
	void InitializeBoss(ACharacter* BossActor);

protected:
	
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	void OnDamageRecived(const FOnAttributeChangeData& 	Data);

	UFUNCTION(BlueprintImplementableEvent, Category = "SF|UI")
	void BP_OnHealthChanged(float Health, float MaxHealth, float Percent);

	UFUNCTION(BlueprintImplementableEvent, Category = "SF|UI")
	void BP_OnNameChanged(const FName& BossName);

	UFUNCTION(BlueprintImplementableEvent, Category = "SF|UI")
	void BP_OnDamageRecived(float Damage);
	
private:

	float CurrentHealth = 1.0f;
	float CurrentMaxHealth = 1.0f;
	TWeakObjectPtr<UAbilitySystemComponent> BossASC;
	
};
