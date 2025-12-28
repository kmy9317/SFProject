#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "SFDeathUIComponent.generated.h"


class UGameOverScreenWidget;
struct FSFGameOverMessage;
class USFPlayerCombatStateComponent;
struct FSFHeroCombatInfo;
class USFDeathScreenWidget;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFDeathUIComponent : public UControllerComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	USFDeathUIComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	static const FName NAME_DeathUIFeature;

	//~ Begin IGameFrameworkInitStateInterface interface
	virtual FName GetFeatureName() const override { return NAME_DeathUIFeature; }
	virtual void CheckDefaultInitialization() override;
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	//~ End IGameFrameworkInitStateInterface interface

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void InitializeDeathSystem(USFPlayerCombatStateComponent* CombatComp);

	// 초기 CombatInfo 도착 시 콜백 (InitState 진행용)
	UFUNCTION()
	void OnInitialCombatInfoReceived(const FSFHeroCombatInfo& CombatInfo);
	
	UFUNCTION()
	void OnCombatInfoChanged(const FSFHeroCombatInfo& CombatInfo);

	UFUNCTION()
	void OnDeathAnimationFinished();

	void ShowDeathScreen();

	void OnResurrected();
	void BindToNewPawnInitState();
	void OnNewPawnGameplayReady(const FActorInitStateChangedParams& Params);

	UFUNCTION()
	void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	void OnGameOver(FGameplayTag Channel, const FSFGameOverMessage& Message);
	void ShowGameOverScreen();

	bool AreAllOtherPlayersIncapacitated();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USFDeathScreenWidget> DeathScreenWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	int32 DeathScreenZOrder = 100;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGameOverScreenWidget> GameOverWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	int32 GameOverScreenZOrder = 500;

private:
	UPROPERTY()
	TObjectPtr<USFDeathScreenWidget> DeathScreenWidget;

	UPROPERTY()
	TObjectPtr<UGameOverScreenWidget> GameOverWidget;

	UPROPERTY()
	TWeakObjectPtr<USFPlayerCombatStateComponent> CachedCombatComp;

	// 마지막으로 처리한 사망 상태 (중복 호출 방지)
	bool bLastKnownDeadState = false;

	bool bIsGameOver = false;

	FGameplayMessageListenerHandle GameOverListenerHandle;

	FDelegateHandle PawnInitStateHandle;
};
