#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Player/SFPlayerInfoTypes.h"
#include "SFHeroDisplay.generated.h"

class UWidgetComponent;
class USFHeroDefinition;
class UCameraComponent;

UCLASS()
class SF_API ASFHeroDisplay : public AActor
{
	GENERATED_BODY()

public:
	ASFHeroDisplay();

	
	void UpdateHeroDefination(const USFHeroDefinition* HeroDefinition);

	/**  PlayerInfo Widget 업데이트(UpdateHeroInfo) */
	void UpdatePlayerInfo(const FSFPlayerInfo& NewPlayerInfo);

	/** PlayerInfo 가져오기 */
	const FSFPlayerInfo& GetPlayerInfo() const { return PlayerInfo; }
	const USFHeroDefinition* GetCurrentHeroDefinition() const { return CurrentHeroDefinition; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
private:
	
	void EnsureWidgetInitialized();
	
	/** PlayerInfo 업데이트 콜백 */
	UFUNCTION()
	void OnRep_PlayerInfo();

	/** HeroDefinition 복제 콜백 */
	UFUNCTION()
	void OnRep_CurrentHeroDefinition();

	/** 위젯에 PlayerInfo 전달 */
	void UpdatePlayerInfoWidget();

	/** 실제 Mesh/Anim 설정 */
	void ApplyHeroConfiguration();
	
private:
	UPROPERTY(VisibleDefaultsOnly, Category = "Character Display")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "Character Display")
	TObjectPtr<UCameraComponent> ViewCameraComponent;

	/** PlayerInfo 위젯 (ScreenSpace) */
	UPROPERTY(VisibleDefaultsOnly, Category = "Character Display")
	TObjectPtr<UWidgetComponent> PlayerInfoWidget;

	/** 복제되는 PlayerInfo (RepNotify) */
	UPROPERTY(ReplicatedUsing=OnRep_PlayerInfo)
	FSFPlayerInfo PlayerInfo;

	/** 복제되는 HeroDefinition (RepNotify) */
	UPROPERTY(ReplicatedUsing=OnRep_CurrentHeroDefinition)
	TObjectPtr<const USFHeroDefinition> CurrentHeroDefinition;
};
