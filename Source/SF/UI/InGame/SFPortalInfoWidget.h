#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "UI/SFUserWidget.h"
#include "SFPortalInfoWidget.generated.h"

class UTextBlock;
class UHorizontalBox;
struct FSFPlayerTravelReadyMessage;
struct FSFPortalStateMessage;
class USFPortalInfoEntryWidget;

/**
 * 
 */
UCLASS()
class SF_API USFPortalInfoWidget : public USFUserWidget
{
	GENERATED_BODY()
	
protected:
	
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** SFGameState->OnPlayerAdded 델리게이트에 연결될 함수 */
	UFUNCTION()
	void HandlePlayerAdded(APlayerState* PlayerState);

	/** SFGameState->OnPlayerRemoved 델리게이트에 연결될 함수 */
	UFUNCTION()
	void HandlePlayerRemoved(APlayerState* PlayerState);

	/** PortalManager의 글로벌 Portal State GMS 메시지를 처리 (UI 표시/숨김/카운트다운) */
	void HandlePortalInfoChanged(FGameplayTag Channel, const FSFPortalStateMessage& Message);

	/** PlayerState의 개별 Ready 상태 GMS 메시지를 처리 (개별 Ready 상태 업데이트) */
	void HandlePlayerReadyChanged(FGameplayTag Channel, const FSFPlayerTravelReadyMessage& Message);

	void UpdateCountdownText();
	
private:
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> PortalEntryBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Countdown;

	UPROPERTY()
	TMap<TWeakObjectPtr<APlayerState>, TObjectPtr<USFPortalInfoEntryWidget>> PortalEntryMap;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Widget")
	TSubclassOf<USFPortalInfoEntryWidget> PortalEntryClass;

	// GMS 리스너 핸들
	FGameplayMessageListenerHandle PortalInfoListenerHandle;
	FGameplayMessageListenerHandle PlayerReadyListenerHandle;

	// 로컬에서 카운트다운 출력용
	float CurrentCountdownTime = 0.0f;
	bool bIsCountingDown = false;
};
