#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "Player/SFPlayerInfoTypes.h"
#include "SFPortalManagerComponent.generated.h"

struct FSFPlayerSelectionInfo;
class ASFPortal;

/**
 * 포탈에서 각 플레이어의 상태
 */
USTRUCT(BlueprintType)
struct FPortalPlayerStatus
{
    GENERATED_BODY()
    
    // 플레이어 정보 (HeroDefinition, Nickname 포함)
    UPROPERTY(BlueprintReadOnly)
    FSFPlayerSelectionInfo PlayerInfo;
    
    // 포탈에 진입했는지 (Ready)
    UPROPERTY(BlueprintReadOnly)
    bool bIsInPortal = false;
    
    FPortalPlayerStatus() = default;
    
    FPortalPlayerStatus(const FSFPlayerSelectionInfo& InPlayerInfo, bool bInPortal)
        : PlayerInfo(InPlayerInfo)
        , bIsInPortal(bInPortal)
    {}
};

/**
 * 포탈의 모든 상태를 포함하는 단일 메시지
 */
USTRUCT(BlueprintType)
struct FSFPortalStateMessage
{
    GENERATED_BODY()
    
    // 포탈 활성화 여부
    UPROPERTY(BlueprintReadOnly)
    bool bIsActive = false;
    
    // 전체 플레이어의 포탈 진입 상태
    UPROPERTY(BlueprintReadOnly)
    TArray<FPortalPlayerStatus> PlayerStatuses;
    
    // 포탈에 진입한 플레이어 수
    UPROPERTY(BlueprintReadOnly)
    int32 PlayersInPortalCount = 0;
    
    // 필요한 플레이어 수 (전체 플레이어 수)
    UPROPERTY(BlueprintReadOnly)
    int32 RequiredPlayerCount = 0;
    
    // Travel 준비 완료 여부 (모든 플레이어 진입)
    UPROPERTY(BlueprintReadOnly)
    bool bReadyToTravel = false;
    
    // Travel 대기 시간 (카운트다운 표시용, -1이면 대기중 아님)
    UPROPERTY(BlueprintReadOnly)
    float TravelCountdown = -1.0f;
};

/**
 * Portal 시스템을 관리하는 GameState Component
 * - Portal 활성화 상태 관리
 * - Portal 내 플레이어 추적
 * - Travel 조건 체크
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFPortalManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
    USFPortalManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
    
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;

    /** Portal 활성화 (스테이지 클리어 시) */
    UFUNCTION(BlueprintCallable, Category = "SF|Portal", BlueprintAuthorityOnly)
    void ActivatePortal();
    
    /** Portal이 활성화되었는지 */
    UFUNCTION(BlueprintPure, Category = "SF|Portal")
    bool IsPortalActive() const { return bPortalActive; }

    /** 플레이어가 Portal에 진입 */
    UFUNCTION(BlueprintCallable, Category = "SF|Portal")
    void NotifyPlayerEnteredPortal(APlayerState* PlayerState);
    
    /** 플레이어가 Portal에서 이탈 */
    UFUNCTION(BlueprintCallable, Category = "SF|Portal")
    void NotifyPlayerLeftPortal(APlayerState* PlayerState);
    
    /** Portal에 있는 플레이어 목록 */
    UFUNCTION(BlueprintPure, Category = "SF|Portal")
    const TArray<FSFPlayerSelectionInfo>& GetPlayersInPortal() const { return PlayersInPortal; }
    
    /** 현재 Portal에 있는 플레이어 수 */
    UFUNCTION(BlueprintPure, Category = "SF|Portal")
    int32 GetCurrentPlayerCount() const { return PlayersInPortal.Num(); }
    
    /** Travel에 필요한 플레이어 수 */
    UFUNCTION(BlueprintPure, Category = "SF|Portal")
    int32 GetRequiredPlayerCount() const;
    
    /** 모든 플레이어가 Portal에 입장했는지 */
    UFUNCTION(BlueprintPure, Category = "SF|Portal")
    bool AreAllPlayersInPortal() const;

    /** Portal Actor 등록 (Portal이 BeginPlay에서 호출) */
    void RegisterPortal(ASFPortal* Portal);
    
    /** Portal Actor 등록 해제 */
    void UnregisterPortal(ASFPortal* Portal);

    /** 관리 중인 Portal 가져오기 */
    UFUNCTION(BlueprintPure, Category = "SF|Portal")
    ASFPortal* GetManagedPortal() const { return ManagedPortal; }

private:
    UFUNCTION()
    void OnRep_PortalActive();
    
    UFUNCTION()
    void OnRep_PlayersInPortal();
    
    /** 포탈 상태 메시지 브로드캐스트 (GameplayMessageSubsystem 사용) */
    void BroadcastPortalState();
    
    /** Portal 준비 체크 및 Travel 시작 */
    void CheckPortalReadyAndTravel();
    
    /** Travel 실행 (타이머 콜백) */
    void ExecuteTravel();
    
    /** PlayerState에서 PlayerSelectionInfo 가져오기 */
    FSFPlayerSelectionInfo GetPlayerSelectionInfo(const APlayerState* PlayerState) const;

private:
    /** Portal 활성화 상태 */
    UPROPERTY(ReplicatedUsing = OnRep_PortalActive)
    bool bPortalActive;
    
    /** Portal에 있는 플레이어 목록 */
    UPROPERTY(ReplicatedUsing = OnRep_PlayersInPortal)
    TArray<FSFPlayerSelectionInfo> PlayersInPortal;
    
    /** Travel 대기 타이머 */
    FTimerHandle TravelTimerHandle;
    
    /** Travel 대기 시간 */
    UPROPERTY(EditDefaultsOnly, Category = "SF|Portal")
    float TravelDelayTime = 3.0f;
    
    /** Travel 카운트다운 남은 시간 (UI용) */
    UPROPERTY()
    float TravelCountdownRemaining = -1.0f;
    
    /** 이미 Travel 중인지 */
    bool bIsTraveling;

    /** 현재 관리중인 Portal */
    UPROPERTY()
    TObjectPtr<ASFPortal> ManagedPortal;
};
