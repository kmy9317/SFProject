#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SFPortal.generated.h"

struct FSFPlayerSelectionInfo;
class UBoxComponent;
class UStaticMeshComponent;
class UParticleSystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPortalPlayerCountChanged, int32, CurrentCount, int32, RequiredCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPortalPlayerEntered, const FSFPlayerSelectionInfo&, PlayerInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPortalPlayerLeft, const FSFPlayerSelectionInfo&, PlayerInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalActivated);

/**
 * 스테이지 전환용 포탈 액터
 * 모든 플레이어가 입장하면 다음 맵으로 이동
 */
UCLASS()
class SF_API ASFPortal : public AActor
{
	GENERATED_BODY()

public:
	ASFPortal();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 포탈 활성화/비활성화(스테이지의 모든 적 처치 후 실행될 예정) */
	UFUNCTION(BlueprintCallable, Category = "SF|Portal")
	void SetPortalEnabled(bool bEnabled);

protected:
	virtual void BeginPlay() override;
	
	/** 현재 입장한 플레이어 수 */
	UFUNCTION(BlueprintPure, Category = "SF|Portal")
	int32 GetCurrentPlayerCount() const { return PlayersInPortal.Num(); }

	/** 필요한 플레이어 수 */
	UFUNCTION(BlueprintPure, Category = "SF|Portal")
	int32 GetRequiredPlayerCount() const;

	/** 포탈 준비 완료 여부 */
	UFUNCTION(BlueprintPure, Category = "SF|Portal")
	bool IsReadyToTravel() const;

	/** 특정 플레이어가 포탈 안에 있는지 확인 */
	UFUNCTION(BlueprintPure, Category = "SF|Portal")
	bool IsPlayerInPortal(const APlayerState* PlayerState) const;

	/** 포탈 내 플레이어 목록 가져오기 */
	UFUNCTION(BlueprintPure, Category = "SF|Portal")
	const TArray<FSFPlayerSelectionInfo>& GetPlayersInPortal() const { return PlayersInPortal; }

protected:
	/** 플레이어 진입 감지 */
	UFUNCTION()
	void OnPortalBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, 
		const FHitResult& SweepResult);

	/** 플레이어 이탈 감지 (TODO : 기획에 따라 삭제 할 수 있음) */
	UFUNCTION()
	void OnPortalEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** 맵 전환 실행 (Server Only) */
	UFUNCTION()
	void TravelToNextStage();

	/** 카운트 변경 시 브로드캐스트 */
	void BroadcastPlayerCountChanged();

	/** 포탈 비주얼 업데이트 (Blueprint 구현) */
	UFUNCTION(BlueprintImplementableEvent, Category = "SF|Portal")
	void OnPortalStateChanged(bool bIsActive, int32 CurrentPlayers, int32 RequiredPlayers);

	UFUNCTION()
	void OnRep_bIsEnabled();

	UFUNCTION()
	void OnRep_PlayersInPortal();

	/** PlayerState에서 FSFPlayerSelectionInfo 가져오기 */
	FSFPlayerSelectionInfo GetPlayerSelectionInfo(const APlayerState* PlayerState) const;

private:
	/** 충돌 감지 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> TriggerBox;

	/** 포탈 메시 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PortalMesh;

	/** 포탈 이펙트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UParticleSystemComponent> PortalEffect;

	/** 다음 스테이지 맵 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SF|Portal", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UWorld> NextStageLevel;

	/** 포탈 활성화 여부 */
	UPROPERTY(ReplicatedUsing = OnRep_bIsEnabled, BlueprintReadOnly, Category = "SF|Portal", meta = (AllowPrivateAccess = "true"))
	bool bIsEnabled;

	/** 포탈 내부의 플레이어 목록 (UI에서 사용할 예정) */
	UPROPERTY(ReplicatedUsing = OnRep_PlayersInPortal)
	TArray<FSFPlayerSelectionInfo> PlayersInPortal;

	/** 자동 전환 대기 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SF|Portal", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float TravelDelayTime;

	/** 전환 대기 타이머 핸들 */
	FTimerHandle TravelTimerHandle;

	/** 이미 전환 중인지 체크 (중복 방지) */
	bool bIsTraveling;

public:
	/** 플레이어 카운트 변경 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "SF|Portal")
	FOnPortalPlayerCountChanged OnPlayerCountChanged;

	/** 플레이어 입장 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "SF|Portal")
	FOnPortalPlayerEntered OnPlayerEntered;

	/** 플레이어 퇴장 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "SF|Portal")
	FOnPortalPlayerLeft OnPlayerLeft;

	/** 포탈 활성화 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "SF|Portal")
	FOnPortalActivated OnPortalActivated;
};
