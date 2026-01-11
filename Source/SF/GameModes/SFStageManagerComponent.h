#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "System/Data/SFStageInfo.h"
#include "System/SFEnemyScalingTypes.h"
#include "SFStageManagerComponent.generated.h"

class USFStageSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStageInfoChangedSignature, const FSFStageInfo&, NewStageInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStageClearedSignature, const FSFStageInfo&, ClearedStageInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossStateChangedSignature, ACharacter*, BossActor);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFStageManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	USFStageManagerComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 현재 스테이지 정보
	UFUNCTION(BlueprintCallable, Category = "SF|Stage")
	const FSFStageInfo& GetCurrentStageInfo() const { return CurrentStageInfo; }

	UFUNCTION(BlueprintCallable, Category = "SF|Stage")
	bool IsStageCleared() const { return bStageCleared; }

	// 스테이지 클리어 처리 (GameMode에서 호출)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|Stage")
	void NotifyStageClear();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|Stage")
	void RegisterBossActor(ACharacter* NewBoss);
	
	UFUNCTION(BlueprintCallable, Category = "SF|Stage")
	ACharacter* GetCurrentBossActor() const { return CurrentBossActor; }

	// ===== Enemy Scaling =====
    
	UFUNCTION(BlueprintPure, Category = "SF|Stage")
	int32 GetPlayerCount() const;

	FSFEnemyScalingContext GetEnemyScalingContext() const;

private:

	UFUNCTION()
	void OnRep_bStageCleared();

public:
	// 스테이지 정보 변경 시 (새 맵 로드 후)
	UPROPERTY(BlueprintAssignable, Category = "SF|Stage|Events")
	FOnStageInfoChangedSignature OnStageInfoChanged;
	
	// 스테이지 클리어 델리게이트 (UI 표시 트리거)
	UPROPERTY(BlueprintAssignable, Category = "SF|Stage|Events")
	FOnStageClearedSignature OnStageCleared;

	UPROPERTY(BlueprintAssignable, Category = "SF|Stage|Events")
	FOnBossStateChangedSignature OnBossStateChanged;

private:
	// 현재 스테이지 정보 (서버 → 클라이언트 복제)
	UPROPERTY(Replicated)
	FSFStageInfo CurrentStageInfo;

	// 스테이지 클리어 여부
	UPROPERTY(ReplicatedUsing = OnRep_bStageCleared)
	bool bStageCleared = false;
	
	UFUNCTION()
	void OnRep_CurrentBossActor();

	UPROPERTY(ReplicatedUsing = OnRep_CurrentBossActor)
	TObjectPtr<ACharacter> CurrentBossActor;

	void SaveLocalPlayerGoldToPlayFab();
};
