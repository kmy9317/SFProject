#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFStatBoostSelectionWidget.generated.h"

struct FSFCommonUpgradeChoice;
class USFStatBoostCardWidget;
class USFCommonUpgradeComponent;
class UButton;
class UTextBlock;

enum class EPendingAction : uint8
{
    None,
    Refresh,    // 일반 리롤
    RefreshWithNotice, // 추가 선택
    Close       // 최종 완료
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatBoostSelectionCardSelected, int32, ChoiceIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatBoostSelectionComplete);

/**
 * 일반 강화 선택 화면 위젯
 */
UCLASS()
class SF_API USFStatBoostSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // 현재 주어진 선택지로  Card 세트 초기화 
    UFUNCTION(BlueprintCallable, Category = "SF|UI")
    void InitializeWithChoices(const TArray<FSFCommonUpgradeChoice>& Choices, int32 NextRerollCost);

    // 선택지 갱신 (리롤시) 
    UFUNCTION(BlueprintCallable, Category = "SF|UI")
    void RefreshChoices(const TArray<FSFCommonUpgradeChoice>& Choices, int32 NextRerollCost);

    // 선택지 갱신 (추가 선택시) 
    UFUNCTION(BlueprintCallable, Category = "SF|UI")
    void RefreshChoicesWithExtraNotice(const TArray<FSFCommonUpgradeChoice>& Choices, int32 NextRerollCost);

    // 선택지 설정 (최초/리롤/추가 선택 시 공통으로 호출)
    UFUNCTION(BlueprintCallable, Category = "SF|UI")
    void SetChoices(const TArray<FSFCommonUpgradeChoice>& Choices);

    // 추가 선택 알림 표시 
    UFUNCTION(BlueprintCallable, Category = "SF|UI")
    void ShowExtraSelectionNotice();

    // 에러 메시지 표시
    UFUNCTION(BlueprintCallable, Category = "SF|UI")
    void ShowErrorMessage(const FText& Message);

    // 추가 선택 알림 표시 (BP에서 구현)
    UFUNCTION(BlueprintImplementableEvent, Category = "SF|UI")
    void BP_ShowExtraSelectionNotice();

    // 에러 메시지 표시 (BP에서 구현) 
    UFUNCTION(BlueprintImplementableEvent, Category = "SF|UI")
    void BP_ShowErrorMessage(const FText& Message);

    // 선택지 갱신 시 (리롤/선택지 추가 등장시) - BP에서 애니메이션 구현 
    UFUNCTION(BlueprintImplementableEvent, Category = "SF|UI")
    void BP_OnChoicesRefreshed();

    UFUNCTION(BlueprintCallable, Category = "SF|UI")
    void NotifyClose();

protected:
    UFUNCTION()
    void OnCardSelected(int32 CardIndex);

    UFUNCTION()
    void OnCardAnimationFinished();

    UFUNCTION()
    void OnRerollClicked();

    void EnableRerollButtonWithDelay();

    void UpdateRerollButtonState();

    UFUNCTION(BlueprintCallable, Category = "SF|UI")
    void UpdateRerollCostDisplay(int32 Cost);

    void UpdateCurrentGoldDisplay();
    
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_CurrentGold;
    
    void DisableAllCards();

    void ProcessPendingAction();

public:
    UPROPERTY(BlueprintAssignable, Category = "SF|UI|Event")
    FOnStatBoostSelectionCardSelected OnCardSelectedDelegate;

    UPROPERTY(BlueprintAssignable, Category = "SF|UI|Event")
    FOnStatBoostSelectionComplete OnSelectionCompleteDelegate;

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<USFStatBoostCardWidget> Card_01;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<USFStatBoostCardWidget> Card_02;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<USFStatBoostCardWidget> Card_03;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Reroll;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_RerollCount;

    // 추가 선택 보너스인 경우 사유 표시 용도
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_ExtraSelectionNotice;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_RerollCost;

    UPROPERTY(Transient, meta = (BindWidgetAnim))
    TObjectPtr<UWidgetAnimation> Anim_ExtraNotice;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SF|UI|Settings")
    float CardRevealDelay = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SF|UI|Settings")
    float RerollButtonEnableDelay = 1.0f;

private:
    TArray<TObjectPtr<USFStatBoostCardWidget>> CardWidgets;
    
    bool bAnimationPlaying = false;

    // 대기중인 서버 응답
    EPendingAction PendingAction = EPendingAction::None;

    // 대기 중인 새 선택지 (추가 보너스 선택용)
    TArray<FSFCommonUpgradeChoice> PendingChoices;

    int32 CachedNextRerollCost = 0;

    // 애니메이션 대기용
    int32 PendingNextRerollCost = 0;  

    FTimerHandle RerollButtonEnableTimerHandle;
};
