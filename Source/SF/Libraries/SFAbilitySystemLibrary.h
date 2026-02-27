#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SFAbilitySystemLibrary.generated.h"

struct FGameplayEventData;
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
	static void SendZeroHealthEventFromSpec(UAbilitySystemComponent* ASC, float Damage, const FGameplayEffectSpec& Spec);
	
	UFUNCTION(BlueprintCallable, Category = "SF|AbilitySystem")
	static void SendHitReactionEventFromSpec(UAbilitySystemComponent* ASC, float Damage, const FGameplayEffectSpec& Spec);

	UFUNCTION(BlueprintCallable, Category = "SF|AbilitySystem")
	static void SendParryEventFromSpec(UAbilitySystemComponent* ASC, float Damage, const FGameplayEffectSpec& Spec);

	UFUNCTION(BlueprintCallable, Category = "SF|AbilitySystem")
	static void SendStaggerEventFromSpec(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec);

	//~=============================================================================
	// Payload 기반 이벤트 (어빌리티에서 주로 사용)
	//~=============================================================================
	
	// 공통 GameplayEvent 발송
	UFUNCTION(BlueprintCallable, Category = "SF|AbilitySystem")
	static void SendGameplayEvent(UAbilitySystemComponent* ASC, const FGameplayTag& EventTag, AActor* Instigator = nullptr);

	// Payload 전달형 (GA_DeathHandler 등에서 원본 컨텍스트를 유지하며 발송)
	UFUNCTION(BlueprintCallable, Category = "SF|AbilitySystem")
	static void SendDeathEvent(UAbilitySystemComponent* ASC, const FGameplayEventData& SourcePayload);

	UFUNCTION(BlueprintCallable, Category = "SF|AbilitySystem")
	static void SendDownedEvent(UAbilitySystemComponent* ASC, const FGameplayEventData& SourcePayload);

};
