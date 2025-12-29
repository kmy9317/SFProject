#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "SFSpectatorComponent.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
struct FSFHeroCombatInfo;
class USFSpectatorHUDWidget;
class ASFSpectatorPawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpectatorTargetChanged, APawn*, NewTarget);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFSpectatorComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	USFSpectatorComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "SF|Spectator")
	void StartSpectating();

	UFUNCTION(BlueprintCallable, Category = "SF|Spectator")
	void StopSpectating();

	UFUNCTION(BlueprintCallable, Category = "SF|Spectator")
	void SpectateNextPlayer();

	UFUNCTION(BlueprintCallable, Category = "SF|Spectator")
	void SpectatePreviousPlayer();

	UFUNCTION(BlueprintPure, Category = "SF|Spectator")
	bool IsSpectating() const { return bIsSpectating; }

	UFUNCTION(BlueprintPure, Category = "SF|Spectator")
	APawn* GetCurrentSpectatorTarget() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 입력 바인딩 초기화 
	void InitializeInputBindings();

	// 관전용 IMC 활성화/비활성화 
	void ToggleInputContext(bool bEnable);

	void TrySpectateNextPlayer();
	
	// 살아있는 타겟 가져오고 정렬(서버와 타겟 대상 동기화를 위해)
	void CollectAliveTargets(TArray<APawn*>& OutTargets) const;
    
	// 특정 타겟으로 즉시 전환
	void SetSpectatorTarget(APawn* NewTarget);

	// 관전 대상의 사망 이벤트 바인딩
	void BindToTargetDeathEvent(APawn* Target);

	// 현재 관전 대상의 이벤트 바인딩 해제
	void UnbindFromCurrentTarget();

	// 관전 대상 사망 시 호출
	UFUNCTION()
	void OnSpectatorTargetCombatInfoChanged(const FSFHeroCombatInfo& CombatInfo);

	UFUNCTION()
	void OnAutoSwitchTimerExpired();

	UFUNCTION(Server, Reliable)
	void Server_SpawnSpectatorPawn();

	UFUNCTION(Server, Reliable)
	void Server_DestroySpectatorPawn();

	UFUNCTION(Server, Reliable)
	void Server_SetViewTarget(APawn* NewTarget);

	void OnSpectatorPawnReady();

	void ShowSpectatorHUD();
	void HideSpectatorHUD();

	void OnSpectateNextInput(const FInputActionValue& Value);
	void OnSpectatePreviousInput(const FInputActionValue& Value);

public:
	UPROPERTY(BlueprintAssignable, Category = "SF|Spectator")
	FOnSpectatorTargetChanged OnSpectatorTargetChanged;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "SF|Spectator")
	TSubclassOf<ASFSpectatorPawn> SpectatorPawnClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "SF|UI")
	TSubclassOf<USFSpectatorHUDWidget> SpectatorHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "SF|UI")
	int32 SpectatorHUDZOrder = 50;

	// 다음 관전 대상 입력 
	UPROPERTY(EditDefaultsOnly, Category = "SF|Input")
	TObjectPtr<UInputAction> SpectateNextAction;

	// 이전 관전 대상 입력 
	UPROPERTY(EditDefaultsOnly, Category = "SF|Input")
	TObjectPtr<UInputAction> SpectatePreviousAction;

	// 관전 모드 전용 IMC 
	UPROPERTY(EditDefaultsOnly, Category = "SF|Input")
	TObjectPtr<UInputMappingContext> SpectatorMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Spectator")
	float ViewTargetBlendTime = 0.2f;

	// 사망 후 자동 전환 대기 시간 
	UPROPERTY(EditDefaultsOnly, Category = "SF|Spectator")
	float AutoSwitchDelay = 2.0f;

protected:
	UFUNCTION()
	void OnRep_SpectatorPawn();

private:
	UPROPERTY(ReplicatedUsing = OnRep_SpectatorPawn)
	TObjectPtr<ASFSpectatorPawn> SpectatorPawn;
		
	UPROPERTY()
	TWeakObjectPtr<APawn> CurrentSpectatorTarget;

	UPROPERTY()
	TWeakObjectPtr<APawn> OriginalPawn;
	
	UPROPERTY()
	TObjectPtr<USFSpectatorHUDWidget> SpectatorHUDWidget;

	bool bIsSpectating = false;
	bool bInputBound = false;

	//자동 전환 타이머 핸들
	FTimerHandle AutoSwitchTimerHandle;

	// 관전 대상을 찾지 못했을 때 재시도하기 위한 타이머 핸들
	FTimerHandle SpectateRetryTimerHandle;
};
