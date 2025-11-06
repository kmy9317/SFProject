#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SFGameMode.generated.h"

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
	// PIE 테스트용 PostLogin(Lobby 없이 InGame 진입시)
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
	virtual void HandleSeamlessTravelPlayer(AController*& Controller) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* Controller) override;
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;

	virtual void StartPlay() override;

	// PlayerState의 PawnData 가져오기
	const USFPawnData* GetPawnDataForController(const AController* InController) const;

private:
	void SetupPlayerPawnDataLoading(APlayerController* PC);
	void OnPlayerPawnDataLoaded(APlayerController* PC, const USFPawnData* PawnData);
	
private:
	// PawnData 로드 대기 중인 플레이어들
	TSet<TWeakObjectPtr<APlayerController>> PendingPlayers;

	// PIE 테스트용 CharacterDefinition
	UPROPERTY(EditDefaultsOnly, Category = "SF|Test", meta = (EditCondition = "bUsePIETestMode"))
	TObjectPtr<USFHeroDefinition> PIETestHeroDefinition;
	
	// PIE 테스트 모드 활성화 여부
	UPROPERTY(EditDefaultsOnly, Category = "SF|Test")
	bool bUsePIETestMode = false;
};
