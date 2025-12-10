#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayEffect.h"
#include "SFWidgetController.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "SFOverlayWidgetController.generated.h"

struct FSFChainStateChangedMessage;
struct FSFPlayerSelectionInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityChangedSignature, FGameplayAbilitySpecHandle, AbilitySpecHandle, bool, bGiven);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChainStateChangedSignature, FGameplayAbilitySpecHandle, AbilitySpecHandle, int32, ChainIndex);

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class SF_API USFOverlayWidgetController : public USFWidgetController
{
	GENERATED_BODY()
public:
	// 초기값 브로드캐스트
	virtual void BroadcastInitialSets() override;

	// 콜백 함수 바인딩
	virtual void BindCallbacksToDependencies() override;

protected:
	UFUNCTION()
	void HandlePlayerInfoChanged(const FSFPlayerSelectionInfo& NewPlayerSelection);

	void HandleAbilityChanged(FGameplayAbilitySpecHandle AbilitySpecHandle, bool bGiven);

	void HandleChainStateChangedMessage(FGameplayTag Channel, const FSFChainStateChangedMessage& Message);
	void HandleGameplayEffectRemoved(const FActiveGameplayEffect& RemovedEffect);

	void BroadcastChainStateForAbility(const UGameplayEffect* EffectDef, int32 StackCount);
private:
	void BindPrimaryAttributeCallbacks();

public:
	// UI가 바인딩할 델리게이트들
	UPROPERTY(BlueprintAssignable, Category="SF|Attributes")
	FOnAttributeChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="SF|Attributes")
	FOnAttributeChangedSignature OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="SF|Attributes")
	FOnAttributeChangedSignature OnManaChanged;

	UPROPERTY(BlueprintAssignable, Category="SF|Attributes")
	FOnAttributeChangedSignature OnMaxManaChanged;

	// UI가 바인딩할 델리게이트들
	UPROPERTY(BlueprintAssignable, Category="SF|Attributes")
	FOnAttributeChangedSignature OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category="SF|Attributes")
	FOnAttributeChangedSignature OnMaxStaminaChanged;

	// FSFPlayerSelectionInfo 구조체 변경을 알릴 델리게이트
	UPROPERTY(BlueprintAssignable, Category="SF|Info")
	FOnPlayerInfoChangedSignature OnPlayerInfoChanged;

	UPROPERTY(BlueprintAssignable, Category="SF|Abilities")
	FOnAbilityChangedSignature OnAbilityChanged;

	UPROPERTY(BlueprintAssignable, Category="SF|Abilities")
	FOnChainStateChangedSignature OnChainStateChanged;

private:
	FGameplayMessageListenerHandle ChainStateListenerHandle;
};
