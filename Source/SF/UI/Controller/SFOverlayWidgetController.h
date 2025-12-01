#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "SFWidgetController.h"
#include "SFOverlayWidgetController.generated.h"

struct FSFPlayerSelectionInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityChangedSignature, FGameplayAbilitySpecHandle, AbilitySpecHandle, bool, bGiven);
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
};
