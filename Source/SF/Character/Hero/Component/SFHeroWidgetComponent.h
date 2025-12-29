#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "SFHeroWidgetComponent.generated.h"

struct FSFHeroCombatInfo;
class USFPlayerCombatStateComponent;
class ASFPlayerState;
struct FOnAttributeChangeData;
class UWidgetComponent;
class USFHeroOverheadWidget;
class UAbilitySystemComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFHeroWidgetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFHeroWidgetComponent(const FObjectInitializer& ObjectInitializer);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 초기화 (Actor에서 호출)
	void SetOverheadWidgetComponent(UWidgetComponent* InWidgetComponent);

	void InitializeHeroStatus(UAbilitySystemComponent* ASC, ASFPlayerState* PS);

	UFUNCTION(BlueprintCallable, Category = "SF|UI")
	void SetPlayerName(const FString& Name);

	USFHeroOverheadWidget* GetOverheadWidget() const;

protected:
	
	//~=============================================================================
	// ASC 관련 바인딩
	//~=============================================================================
	
	void InitializeWithASC(UAbilitySystemComponent* ASC);
	void UninitializeASC();
		
	void BindTagEvents();
	void UnbindTagEvents();
	void OnDownedTagChanged(const FGameplayTag Tag, int32 NewCount);

	// Attribute 바인딩 (Downed 상태일 때만)
	void BindReviveGaugeAttribute();
	void UnbindReviveGaugeAttribute();
	void OnReviveGaugeChanged(const FOnAttributeChangeData& Data);

	// UI 제어
	void OnDownedStateBegin();
	void OnDownedStateEnd();

	//~=============================================================================
	// CombatState 관련 바인딩
	//~=============================================================================
	
	void InitializeWithCombatState(USFPlayerCombatStateComponent* CombatStateComp);
	void UninitializeCombatState();
    
	UFUNCTION()
	void OnCombatInfoChanged(const FSFHeroCombatInfo& CombatInfo);

	// UI 업데이트
	void UpdateCombatInfoUI(const FSFHeroCombatInfo& CombatInfo);

protected:
	// 관리중인 위젯 컴포넌트 참조 
	UPROPERTY()
	TWeakObjectPtr<UWidgetComponent> OverheadWidgetComponent;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	UPROPERTY()
	TWeakObjectPtr<ASFPlayerState> CachedPS;
	
	UPROPERTY()
	TWeakObjectPtr<USFPlayerCombatStateComponent> CachedCombatStateComponent;
	
	// 바인딩 핸들
	FDelegateHandle DownedTagDelegateHandle;
	FDelegateHandle ReviveGaugeDelegateHandle;
};
