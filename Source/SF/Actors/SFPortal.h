#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "SFPortal.generated.h"

class USFPortalManagerComponent;
class UBoxComponent;
class UStaticMeshComponent;
class UParticleSystemComponent;

/**
 * 물리적 포탈 트리거
 * Overlap 감지하고 GameState에 알림
 */
UCLASS()
class SF_API ASFPortal : public AActor
{
	GENERATED_BODY()

public:
	ASFPortal();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Portal 활성화/비활성화 */
	UFUNCTION(BlueprintCallable, Category = "SF|Portal")
	void SetPortalEnabled(bool bEnabled);

	/** Portal 활성화 여부 */
	UFUNCTION(BlueprintPure, Category = "SF|Portal")
	bool IsEnabled() const { return bIsEnabled; }

	/** 다음 스테이지 레벨 가져오기 */
	UFUNCTION(BlueprintPure, Category = "SF|Portal")
	TSoftObjectPtr<UWorld> GetNextStageLevel() const { return NextStageLevel; }

	UPrimitiveComponent* GetTriggerComponent() const { return TriggerBox; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	/** 플레이어 진입 감지 */
	UFUNCTION()
	void OnPortalBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, 
		const FHitResult& SweepResult);

	/** 플레이어 이탈 감지 */
	UFUNCTION()
	void OnPortalEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_bIsEnabled();

private:
	/** PortalManager 찾기 및 등록 */
	void FindAndRegisterWithManager();

private:
	/** 충돌 감지 컴포넌트 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	/** 포탈 메시 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PortalMesh;

	/** 포탈 이펙트 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UParticleSystemComponent> PortalEffect;

	/** 다음 스테이지 레벨 */
	UPROPERTY(EditInstanceOnly, Category = "SF|Portal", meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> NextStageLevel;

	/** Portal 활성화 여부 */
	UPROPERTY(ReplicatedUsing = OnRep_bIsEnabled)
	bool bIsEnabled;

	/** 캐시된 PortalManager */
	UPROPERTY()
	TObjectPtr<USFPortalManagerComponent> CachedPortalManager;
};
