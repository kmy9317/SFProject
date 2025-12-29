#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFDeathScreenWidget.generated.h"

class USFSpectatorComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathAnimationFinished);

/**
 * 
 */
UCLASS()
class SF_API USFDeathScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SF|UI")
	void PlayDeathDirection();

protected:
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

public:
	UPROPERTY(BlueprintAssignable, Category = "UI|Death")
	FOnDeathAnimationFinished OnDeathAnimationFinished;

protected:
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_Death;

};
