#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SFPortal.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UParticleSystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPortalPlayerCountChanged, int32, CurrentCount, int32, RequiredCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalActivated);

UCLASS()
class SF_API ASFPortal : public AActor
{
	GENERATED_BODY()

public:
	ASFPortal();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:
	virtual void BeginPlay() override;

	/** 포탈 활성화/비활성화 */
	UFUNCTION(BlueprintCallable, Category = "SF|Portal")
	void SetPortalEnabled(bool bEnabled);

	/** 현재 입장한 플레이어 수 */
	UFUNCTION(BlueprintPure, Category = "SF|Portal")
	int32 GetCurrentPlayerCount() const { return PlayersInPortal.Num(); }

	/** 필요한 플레이어 수 */
	UFUNCTION(BlueprintPure, Category = "SF|Portal")
	int32 GetRequiredPlayerCount() const;

	/** 포탈 준비 완료 여부 */
	UFUNCTION(BlueprintPure, Category = "SF|Portal")
	bool IsReadyToTravel() const;

protected:
	/** 플레이어 진입 감지 */
	UFUNCTION()
	void OnPortalBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, 
		const FHitResult& SweepResult);

	/** 플레이어 이탈 감지 */
	UFUNCTION()
	void OnPortalEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** 맵 전환 실행 (Server Only) */
	UFUNCTION()
	void TravelToNextStage();

	/** 카운트 변경 시 브로드캐스트 */
	void BroadcastPlayerCountChanged();

	/** 포탈 비주얼 업데이트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "SF|Portal")
	void OnPortalStateChanged(bool bIsActive, int32 CurrentPlayers, int32 RequiredPlayers);

	UFUNCTION()
	void OnRep_bIsEnabled();

	UFUNCTION()
	void OnRep_PlayersInPortal();

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SF|Level", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UWorld> NextStageLevel;

	/** 포탈 활성화 여부 */
	UPROPERTY(ReplicatedUsing = OnRep_bIsEnabled, BlueprintReadOnly, Category = "SF|Portal", meta = (AllowPrivateAccess = "true"))
	bool bIsEnabled;

	/** 포탈 내부의 플레이어 목록 */
	UPROPERTY(ReplicatedUsing = OnRep_PlayersInPortal)
	TArray<TObjectPtr<APlayerController>> PlayersInPortal;

	/** 자동 전환 대기 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SF|Portal", meta = (AllowPrivateAccess = "true"))
	float TravelDelayTime;

	/** 전환 대기 타이머 핸들 */
	FTimerHandle TravelTimerHandle;

	/** 이미 전환 중인지 체크 (중복 방지) */
	bool bIsTraveling;

public:
	/** 플레이어 카운트 변경 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "SF|Portal")
	FOnPortalPlayerCountChanged OnPlayerCountChanged;

	/** 포탈 활성화 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "SF|Portal")
	FOnPortalActivated OnPortalActivated;
};
