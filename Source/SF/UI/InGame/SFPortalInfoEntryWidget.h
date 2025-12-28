#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidget.h"
#include "SFPortalInfoEntryWidget.generated.h"

struct FSFPlayerSelectionInfo;
struct FStreamableHandle;
class UImage;
class UTextBlock;
class ASFPlayerState;

/**
 * PortalInfoWidget 목록에 표시될 개별 플레이어 항목 위젯
 * 플레이어 닉네임, 영웅 아이콘, 맵 이동 준비 상태를 표시
 */
UCLASS()
class SF_API USFPortalInfoEntryWidget : public USFUserWidget
{
	GENERATED_BODY()

public:
	/** 위젯 엔트리를 특정 PlayerState와 매핑 */
	void InitializeRow(ASFPlayerState* OwningPlayerState);

	void SetReadyStatus(bool bIsReady);

	void SetDeadStatus(bool bIsDead);

protected:
	virtual void NativeDestruct() override;

	// 이 파티원의 PlayerState에 있는 OnRep_PlayerSelectionInfo가 호출될 때 실행될 핸들러
	UFUNCTION()
	void HandlePlayerInfoChanged(const FSFPlayerSelectionInfo& PlayerInfo);

	// 아이콘 비동기 로드 완료시 호출
	void OnIconLoadCompleted();
	
protected:
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_PlayerName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_HeroIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_ReadyCheck;

private:
	TWeakObjectPtr<ASFPlayerState> CachedPlayerState;

	TSharedPtr<FStreamableHandle> IconLoadHandle;

	bool bIsPlayerDead = false;
};
