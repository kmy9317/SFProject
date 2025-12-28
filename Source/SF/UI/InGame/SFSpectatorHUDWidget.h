#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFSpectatorHUDWidget.generated.h"

class ASFPlayerState;
struct FSFPlayerSelectionInfo;
class USFSpectatorComponent;
class UTextBlock;

UCLASS()
class SF_API USFSpectatorHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnSpectatorTargetChanged(APawn* NewTarget);

	UFUNCTION()
	void OnTargetPlayerInfoChanged(const FSFPlayerSelectionInfo& NewPlayerSelection);

	UFUNCTION(BlueprintPure, Category = "UI|Spectator")
	USFSpectatorComponent* GetSpectatorComponent() const;

	void UpdateTargetName(ASFPlayerState* SFPS);
	void UnbindFromCurrentTarget();

protected:
	// 관전 대상 이름
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SpectatingPlayerName;

private:
	UPROPERTY()
	mutable TWeakObjectPtr<USFSpectatorComponent> CachedSpectatorComponent;

	UPROPERTY()
	TWeakObjectPtr<ASFPlayerState> CachedTargetPlayerState;
};
