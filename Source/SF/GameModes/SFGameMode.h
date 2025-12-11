#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SFGameMode.generated.h"

class ASFPortal;
class USFHeroDefinition;
class USFPawnData;

/**
 * 
 */
UCLASS()
class SF_API ASFGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASFGameMode();
	
	// PIE 테스트용 PostLogin(Lobby 없이 InGame 진입시)
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void HandleSeamlessTravelPlayer(AController*& Controller) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* Controller) override;
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	virtual void StartPlay() override;

	/**  PlayerState의 PawnData 가져오기*/
	const USFPawnData* GetPawnDataForController(const AController* InController) const;

	/** 스테이지 클리어 시 포탈 활성화 */
	UFUNCTION(BlueprintCallable, Category = "SF|GameMode")
	void ActivatePortal();

	/** GameState에서 호출하는 Travel 요청 */
	void RequestTravelToNextStage(TSoftObjectPtr<UWorld> NextStageLevel);

protected:
	UFUNCTION()
	void OnAllEnemiesDefeated();

private:
	void SetupPlayerPawnDataLoading(APlayerController* PC);
	void OnPlayerPawnDataLoaded(APlayerController* PC, const USFPawnData* PawnData);

private:
	/** PawnData 로드 대기 중인 플레이어들*/
	TSet<TWeakObjectPtr<APlayerController>> PendingPlayers;

	/**PIE 테스트용 CharacterDefinition*/
	UPROPERTY(EditDefaultsOnly, Category = "SF|Test", meta = (EditCondition = "bUsePIETestMode"))
	TObjectPtr<USFHeroDefinition> PIETestHeroDefinition;

	/** PIE 테스트 모드 활성화 여부*/
	UPROPERTY(EditDefaultsOnly, Category = "SF|Test")
	bool bUsePIETestMode = false;

	/** TODO : 테스트용 자동 포탈 활성화 여부 (삭제 예정)*/
	UPROPERTY(EditDefaultsOnly, Category = "SF|Test|Portal")
	bool bAutoActivatePortal = true;

	/** TODO : 게임 시작 후 포탈 활성화까지 대기 시간 (삭제 예정)*/
	UPROPERTY(EditDefaultsOnly, Category = "SF|Test|Portal", meta = (EditCondition = "bAutoActivatePortal", ClampMin = "0.0"))
	float PortalActivationDelay = 5.0f;

	FTimerHandle PortalActivationTimerHandle;

	UPROPERTY()
	TArray<APlayerStart*> AssignedPlayerStarts;
};
