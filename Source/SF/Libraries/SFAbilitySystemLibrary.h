#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SFAbilitySystemLibrary.generated.h"

struct FGameplayTag;
struct FGameplayEffectSpec;
class UAbilitySystemComponent;

/**
 * 
 */
UCLASS()
class SF_API USFAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	//~=============================================================================
	// Spec 기반 이벤트 (AttributeSet에서 주로 사용)
	//~=============================================================================

	// 공통 GameplayEvent 발송 (Spec 기반) 
	UFUNCTION(BlueprintCallable, Category = "SF|AbilitySystem")
	static void SendGameplayEventFromSpec(UAbilitySystemComponent* ASC, const FGameplayTag& EventTag, const FGameplayEffectSpec& Spec);

	UFUNCTION(BlueprintCallable, Category = "SF|AbilitySystem")
	static void SendDeathEventFromSpec(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec);

	UFUNCTION(BlueprintCallable, Category = "SF|AbilitySystem")
	static void SendDownedEventFromSpec(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec);

	UFUNCTION(BlueprintCallable, Category = "SF|AbilitySystem")
	static void SendHitReactionEventFromSpec(UAbilitySystemComponent* ASC, float Damage, const FGameplayEffectSpec& Spec);

	UFUNCTION(BlueprintCallable, Category = "SF|AbilitySystem")
	static void SendParryEventFromSpec(UAbilitySystemComponent* ASC, float Damage, const FGameplayEffectSpec& Spec);

	UFUNCTION(BlueprintCallable, Category = "SF|AbilitySystem")
	static void SendStaggerEventFromSpec(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec);

	//~=============================================================================
	// Spec 없는 이벤트 (어빌리티 등에서 사용)
	//~=============================================================================
	
	// 공통 GameplayEvent 발송 (Spec 없는 버전) 
	UFUNCTION(BlueprintCallable, Category = "SF|AbilitySystem")
	static void SendGameplayEvent(UAbilitySystemComponent* ASC, const FGameplayTag& EventTag, AActor* Instigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "SF|AbilitySystem")
	static void SendDeathEvent(UAbilitySystemComponent* ASC, AActor* Instigator = nullptr);
};
