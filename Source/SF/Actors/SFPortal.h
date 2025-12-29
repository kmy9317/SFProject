#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "Interaction/SFInteractable.h"
#include "SFPortal.generated.h"

class USFPortalManagerComponent;
class UBoxComponent;
class UStaticMeshComponent;
class UNiagaraComponent;

/**
 * 물리적 포탈 트리거
 * Overlap 감지하고 GameState에 알림
 */
UCLASS()
class SF_API ASFPortal : public AActor, public ISFInteractable
{
	GENERATED_BODY()

public:
	ASFPortal();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ~ Begin ISFInteractable
	virtual FSFInteractionInfo GetPreInteractionInfo(const FSFInteractionQuery& InteractionQuery) const override;
	virtual bool CanInteraction(const FSFInteractionQuery& InteractionQuery) const override;
	// ~ End ISFInteractable

	/** Portal 활성화/비활성화 */
	UFUNCTION(BlueprintCallable, Category = "SF|Portal")
	void SetPortalEnabled(bool bEnabled);

	/** Portal 활성화 여부 */
	UFUNCTION(BlueprintPure, Category = "SF|Portal")
	bool IsEnabled() const { return bIsEnabled; }

	/** 다음 스테이지 레벨 가져오기 */
	UFUNCTION(BlueprintPure, Category = "SF|Portal")
	TSoftObjectPtr<UWorld> GetNextStageLevel() const { return NextStageLevel; }

	/** 플레이어 Ready 토글 (서버에서 호출) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|Portal")
	void TogglePlayerReady(APlayerState* PlayerState);

	/** 플레이어 Ready 상태 확인 */
	UFUNCTION(BlueprintPure, Category = "SF|Portal")
	bool IsPlayerReady(APlayerState* PlayerState) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UFUNCTION()
	void OnRep_bIsEnabled();

private:
	/** PortalManager 찾기 및 등록 */
	void FindAndRegisterWithManager();
	void UpdatePortalEffects();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayActivateSound();

public:
	/** 포탈 이펙트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> PortalEffect;
	
private:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> InteractionBox;
	
	/** 포탈 메시 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PortalMesh;

	/** 다음 스테이지 레벨 */
	UPROPERTY(EditInstanceOnly, Category = "SF|Portal", meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> NextStageLevel;

	UPROPERTY(EditAnywhere, Category = "SF|Portal|Sound")
	TObjectPtr<USoundBase> PortalActivateSound;

	UPROPERTY(EditAnywhere, Category = "SF|Portal|Sound")
	TObjectPtr<USoundBase> PortalLoopSound;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> LoopAudioComponent;

	/** Portal 활성화 여부 */
	UPROPERTY(ReplicatedUsing = OnRep_bIsEnabled)
	uint8  bIsEnabled : 1;

	/** "이동 준비" 상호작용 정보 */
	UPROPERTY(EditAnywhere, Category = "SF|Portal|Interaction")
	FSFInteractionInfo ReadyInteractionInfo;

	/** "준비 취소" 상호작용 정보 */
	UPROPERTY(EditAnywhere, Category = "SF|Portal|Interaction")
	FSFInteractionInfo CancelReadyInteractionInfo;

	/** 캐시된 PortalManager */
	UPROPERTY()
	TObjectPtr<USFPortalManagerComponent> CachedPortalManager;
};
