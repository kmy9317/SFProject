#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "Messages/SFPortalInfoMessages.h"
#include "SFPortalManagerComponent.generated.h"

struct FSFPlayerSelectionInfo;
class ASFPortal;

/**
 * Portal 시스템을 관리하는 GameState Component
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFPortalManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
    USFPortalManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
    
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Portal 활성화 (스테이지 클리어 시) */
    UFUNCTION(BlueprintCallable, Category = "SF|Portal", BlueprintAuthorityOnly)
    void ActivatePortal();

    /** 플레이어가 Portal에 진입 (서버 전용) */
    UFUNCTION(BlueprintCallable, Category = "SF|Portal")
    void NotifyPlayerEnteredPortal(APlayerState* PlayerState);
    
    /** 플레이어가 Portal에서 이탈 (서버 전용) */
    UFUNCTION(BlueprintCallable, Category = "SF|Portal")
    void NotifyPlayerLeftPortal(APlayerState* PlayerState);
    
    /** Portal Actor 등록 (World에 배치된 Portal이 BeginPlay에서 호출) */
    void RegisterPortal(ASFPortal* Portal);
    
    /** Portal Actor 등록 해제 */
    void UnregisterPortal(ASFPortal* Portal);

    int32 GetPlayersReadyCount();

private:

    /** 현재 포탈 내부에 있는 플레이어들을 수동으로 다시 체크 */
    void RecheckExistingOverlaps();
    
    /** 포탈 상태 메시지 브로드캐스트 */
    void BroadcastPortalState();
    
    /** Portal 준비 체크 및 Travel 시작 */
    void CheckPortalReadyAndTravel();
    
    /** Travel 실행 (타이머 콜백) */
    void ExecuteTravel();

    /** 필요한 플레이어 수 계산 */
    int32 GetRequiredPlayerCount() const;

    /** 중간에 나간 플레이어 case 처리 */
    UFUNCTION()
    void HandlePlayerRemoved(APlayerState* PlayerState);
    
    UFUNCTION()
    void OnRep_PortalActive();

    UFUNCTION()
    void OnRep_PortalStateChanged();

private:
    /** Portal 활성화 상태 */
    UPROPERTY(ReplicatedUsing = OnRep_PortalActive)
    bool bPortalActive;

    /** 포털 UI 상태 (전역) */
    UPROPERTY(ReplicatedUsing = OnRep_PortalStateChanged)
    FSFPortalStateMessage PortalState;

    /** Travel 대기 시간 */
    UPROPERTY(EditDefaultsOnly, Category = "SF|Portal")
    float TravelDelayTime = 5.0f;
    
    /** Travel 대기 타이머 */
    FTimerHandle TravelTimerHandle;
    
    /** Travel 중인지 */
    bool bIsPrepareToTravel;;

    /** 현재 관리중인 Portal */
    UPROPERTY()
    TObjectPtr<ASFPortal> ManagedPortal;
};
