#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "System/Data/SFStageInfo.h"
#include "SFPlayerController.generated.h"

class USFSharedUIComponent;
class USFDeathUIComponent;
class USFSpectatorComponent;
struct FSFStageInfo;
class USFSkillSelectionScreen;
class USFLoadingCheckComponent;
class ASFPlayerState;
class USFAbilitySystemComponent;
class UUserWidget;
class UInputAction;
class UInputMappingContext;

/**
 * 
 */
UCLASS()
class SF_API ASFPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASFPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AController interface
	virtual void BeginPlay() override;
	virtual void OnUnPossess() override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupInputComponent() override; 
	//~End of AController interface

	//~APlayerController interface
	virtual void PlayerTick(float DeltaTime) override;
	//~End of APlayerController interface

	UFUNCTION(BlueprintCallable, Category = "SF|PlayerController")
	ASFPlayerState* GetSFPlayerState() const;
	
	UFUNCTION(BlueprintCallable, Category = "SF|PlayerController")
	USFAbilitySystemComponent* GetSFAbilitySystemComponent() const;

	// 로비 복귀 준비 완료 
	UFUNCTION(BlueprintCallable, Category = "SF|GameOver")
	void RequestReadyForLobby();
  
public:
  //영구강화
  UFUNCTION(Server, Reliable)
  void Server_SendPermanentUpgradeData(const FSFPermanentUpgradeData& InData);

  UFUNCTION(Client, Reliable)
  void Client_BeginPermanentUpgradeFlow();


protected:
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	
	UFUNCTION(Server, Unreliable)
	void Server_UpdateViewRotation(FRotator NewRotation);

	// 인게임 메뉴 생성 함수
	void ToggleInGameMenu();
	// 팀원 위젯 생성 함수
	void CreateTeammateIndicators();

	UFUNCTION(Server, Reliable)
	void Server_NotifyReadyForLobby();

protected:
	// ----------------[추가] 인게임 메뉴 관련 변수 및 함수----------------------

	// 에디터 상에서 지정할 IA_InGameMenu 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Input")
	TObjectPtr<UInputAction> InGameMenuAction;

	// 에디터 상에서 지정할 IMC 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|InGame")
	TSubclassOf<UUserWidget> InGameMenuClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> InGameMenuInstance;

	// 팀원 표시 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|InGame")
	TSubclassOf<UUserWidget> TeammateIndicatorWidgetClass;

	// 생성된 팀원 표시 위젯 관리 맵
	UPROPERTY()
	TMap<AActor*, class USFIndicatorWidgetBase*> TeammateWidgetMap;

	// 팀원 표시 검색 타이머 핸들
	FTimerHandle TeammateSearchTimerHandle;
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFLoadingCheckComponent> LoadingCheckComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFSpectatorComponent> SpectatorComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFDeathUIComponent> DeathUIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFSharedUIComponent> SharedUIComponent;

private:
	// ViewRotation 전송 최적화 
	FRotator LastSentViewRotation;
	float LastViewRotationSendTime = 0.f;

	// 회전 변경 감지 임계값
	static constexpr float ViewRotationThreshold = 1.0f;

	// 최소 전송 간격
	static constexpr float ViewRotationSendInterval = 0.05f;

};
