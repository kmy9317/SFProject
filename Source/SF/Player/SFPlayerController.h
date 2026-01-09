#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "System/Data/SFStageInfo.h"
#include "UI/InGame/UIDataStructs.h"
#include "SFPlayerController.generated.h"

class USFBossHUDWidget;
class USFQuickbarComponent;
class USFItemManagerComponent;
class USFInventoryManagerComponent;
class USFSharedUIComponent;
class USFDeathUIComponent;
class USFSpectatorComponent;
class USFInGameMenuComponent;
struct FSFStageInfo;
class USFSkillSelectionScreen;
class USFLoadingCheckComponent;
class ASFPlayerState;
class USFAbilitySystemComponent;
class UUserWidget;
class USFDamageWidget;

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

	UFUNCTION(BlueprintCallable, Category = "SF|GameOver")
	void CreateBossHUD();

	UFUNCTION(BlueprintCallable, Category = "SF|GameOver")
	void RemoveBossHUD(ACharacter* BossActor);
	
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
	
	// 팀원 위젯 생성 함수
	void CreateTeammateIndicators();
	
	// 몬스터 데미지 텍스트 메세지 함수 (서버 실행)
	void OnDamageMessageReceived(FGameplayTag Channel, const FSFDamageMessageInfo& Payload);
	
	// 클라이언트 데미지 텍스트 출력 RPC 함수
	UFUNCTION(Client, Unreliable)
	void Client_ShowDamageText(float DamageAmount, AActor* TargetActor);
	
	// 게임 졸료 시 리스너 해제 함수 
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(Server, Reliable)
	void Server_NotifyReadyForLobby();

protected:
	// 팀원 표시 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|InGame")
	TSubclassOf<UUserWidget> TeammateIndicatorWidgetClass;
	
	// 생성된 팀원 표시 위젯 관리 맵
	UPROPERTY()
	TMap<AActor*, class USFIndicatorWidgetBase*> TeammateWidgetMap;

	// 팀원 표시 검색 타이머 핸들
	FTimerHandle TeammateSearchTimerHandle;

	// 보스전 전용 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|InGame")
	TSubclassOf<USFBossHUDWidget> BossHUDWidgetClass;

	UPROPERTY()
	USFBossHUDWidget* BossHUDWidgetInstance;
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFLoadingCheckComponent> LoadingCheckComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFSpectatorComponent> SpectatorComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFDeathUIComponent> DeathUIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFSharedUIComponent> SharedUIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFInventoryManagerComponent> InventoryManagerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFQuickbarComponent> QuickbarComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFItemManagerComponent> ItemManagerComponent;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFInGameMenuComponent> InGameMenuComponent; 

private:
	// ViewRotation 전송 최적화 
	FRotator LastSentViewRotation;
	float LastViewRotationSendTime = 0.f;

	// 회전 변경 감지 임계값
	static constexpr float ViewRotationThreshold = 1.0f;

	// 최소 전송 간격
	static constexpr float ViewRotationSendInterval = 0.05f;
	
	// 리스너 등록증(핸들) 저장 변수
	FGameplayMessageListenerHandle DamageMessageListenerHandle;

public:
	// 몬스터 데미지 텍스트 위젯(WBP) 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|InGame")
	TSubclassOf<USFDamageWidget> DamageWidgetClass;
};
