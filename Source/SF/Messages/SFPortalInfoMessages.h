#pragma once

#include "CoreMinimal.h"
#include "SFPortalInfoMessages.generated.h"

/**
 * 포탈의 모든 상태를 포함하는 단일 메시지
 */
USTRUCT(BlueprintType)
struct FSFPortalStateMessage
{
	GENERATED_BODY()
    
	// UI Visible 여부
	UPROPERTY(BlueprintReadOnly)
	bool bIsActive = false;
    
	// 전체 플레이어 수
	UPROPERTY(BlueprintReadOnly)
	int32 TotalPlayerCount = 0;

	// Travel 대기 시간 (음수면 카운트다운 안 함)
	UPROPERTY(BlueprintReadOnly)
	float TravelCountdown = -1.0f;
};

/**
 * 개별 플레이어의 맵 이동 준비 상태가 변경 메시지
 */
USTRUCT(BlueprintType)
struct FSFPlayerTravelReadyMessage
{
	GENERATED_BODY()

	/** 상태가 변경된 플레이어의 PlayerState */
	UPROPERTY(BlueprintReadOnly, Category = "GameplayMessage")
	TObjectPtr<APlayerState> PlayerState = nullptr;
	
	/** 맵 이동 준비 상태 */	
	UPROPERTY(BlueprintReadOnly, Category = "GameplayMessage")
	bool bIsReadyToTravel = false;
};

/**
 * 플레이어 Dead 상태 변경 메시지
 */
USTRUCT(BlueprintType)
struct FSFPlayerDeadStateMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APlayerState> PlayerState = nullptr;

	UPROPERTY(BlueprintReadOnly)
	bool bIsDead = false;
};

USTRUCT(BlueprintType)
struct FSFPlayerDownedStateMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APlayerState> PlayerState = nullptr;

	UPROPERTY(BlueprintReadOnly)
	bool bIsDowned = false;
};

USTRUCT(BlueprintType)
struct FSFGameOverMessage
{
	GENERATED_BODY()

	// 추후 확장용 (예: 게임오버 사유, 통계 등)
	UPROPERTY(BlueprintReadOnly)
	float SurvivedTime = 0.f;
};

USTRUCT(BlueprintType)
struct FSFLobbyReadyMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 ReadyCount = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 TotalCount = 0;
};