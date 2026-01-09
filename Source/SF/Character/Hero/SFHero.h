// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/SFCharacterBase.h"
#include "SFHero.generated.h"

class USFHeroWidgetComponent;
class UWidgetComponent;
class USFHeroOverheadWidget;
class ASFPlayerController;
/**
 * 
 */
UCLASS()
class SF_API ASFHero : public ASFCharacterBase
{
	GENERATED_BODY()
public:
	ASFHero(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "SF|Hero")
	ASFPlayerController* GetSFPlayerController() const;
	
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override {}

	// ~ Begin ISFInteractable
	virtual FSFInteractionInfo GetPreInteractionInfo(const FSFInteractionQuery& InteractionQuery) const override;
	virtual bool CanInteraction(const FSFInteractionQuery& InteractionQuery) const override;
	virtual void OnInteractActiveStarted(AActor* Interactor) override;
	virtual void OnInteractActiveEnded(AActor* Interactor) override;
	virtual void OnInteractionSuccess(AActor* Interactor) override;
	virtual int32 GetActiveInteractorCount() const override;
	virtual ESFOutlineStencil GetOutlineStencil() const override;
	// ~ End ISFInteractable

	const TArray<TWeakObjectPtr<AActor>>& GetCachedRevivers() const { return CachedRevivers; }
	USFHeroWidgetComponent* GetHeroWidgetComponent() const { return HeroWidgetComponent; }

protected:
	virtual void OnAbilitySystemInitialized() override;
	virtual void OnAbilitySystemUninitialized() override;

	void OnDownedTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "SF|Revive")
	FSFInteractionInfo ReviveInteractionInfo;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|UI")
	TObjectPtr<UWidgetComponent> OverheadWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|UI")
	TObjectPtr<USFHeroWidgetComponent> HeroWidgetComponent;

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> CachedRevivers;

	FDelegateHandle DownedTagDelegateHandle;
};
