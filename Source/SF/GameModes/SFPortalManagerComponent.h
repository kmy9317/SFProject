#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
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

    /** 플레이어 Ready 상태 토글 */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|Portal")
    void TogglePlayerReady(APlayerState* PlayerState);
    
    void RegisterPortal(ASFPortal* Portal);
    void UnregisterPortal(ASFPortal* Portal);

    UFUNCTION(BlueprintPure, Category = "SF|Portal")
    bool IsTravelCountdownActive() const { return PortalState.bIsActive && PortalState.TravelCountdown <= TravelDelayTime; }

    // Ready 상태인 살아있는 플레이어 수
    UFUNCTION(BlueprintPure, Category = "SF|Portal")
    int32 GetReadyPlayerCount() const;

    UFUNCTION(BlueprintPure, Category = "SF|Portal")
    int32 GetRequiredPlayerCount() const;

    UFUNCTION(BlueprintPure, Category = "SF|Portal")
    ASFPortal* GetManagedPortal() const { return ManagedPortal; }

private:

    void OnFirstPlayerReady();
    void CheckAllPlayersReady();
    void StartTravelCountdown();
    void ExecuteTravel();
    void BroadcastPortalState();

    UFUNCTION()
    void HandlePlayerRemoved(APlayerState* PlayerState);
    
    UFUNCTION()
    void OnRep_PortalActive();

    UFUNCTION()
    void OnRep_PortalState();

    void OnPlayerDeadStateChanged(FGameplayTag Channel, const FSFPlayerDeadStateMessage& Message);

private:
    /** Portal 활성화 상태 */
    UPROPERTY(ReplicatedUsing = OnRep_PortalActive)
    bool bPortalActive = false;

    /** 포털 UI 상태 (전역) */
    UPROPERTY(ReplicatedUsing = OnRep_PortalState)
    FSFPortalStateMessage PortalState;

    /** 첫 Ready 후 강제 Travel까지 시간 */
    UPROPERTY(EditDefaultsOnly, Category = "SF|Portal")
    float ForceTimeLimit = 120.0f;

    /** 전원 Ready 후 Travel까지 대기 시간 */
    UPROPERTY(EditDefaultsOnly, Category = "SF|Portal")
    float TravelDelayTime = 5.0f;
    
    FTimerHandle ForceTimerHandle;
    FTimerHandle TravelTimerHandle;

    bool bIsTravelCountdownActive = false;
    
    bool bLoadingScreenPreloaded = false;

    UPROPERTY()
    TObjectPtr<ASFPortal> ManagedPortal;

    FGameplayMessageListenerHandle DeadStateListenerHandle;
};
