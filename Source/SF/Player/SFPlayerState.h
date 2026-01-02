#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "SFPlayerInfoTypes.h"
#include "GameFramework/PlayerState.h"
#include "Save/SFPersistentDataType.h"
#include "Team/SFTeamTypes.h"
#include "Character/Hero/Component/SFPermanentUpgradeComponent.h"
#include "Components/SFPlayerCombatStateComponent.h"
#include "System/Data/SFPermanentUpgradeTypes.h"
#include "SFPlayerState.generated.h"

class USFCommonUpgradeComponent;
class USFGameplayAbility;
class USFCombatSet_Hero;
class USFPrimarySet_Hero;
struct FStreamableHandle;
class ASFPlayerController;
class USFPawnData;
class USFAbilitySystemComponent;
class USFPlayerCombatStateComponent;

// PawnData 로드 완료 델리게이트
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPawnDataLoaded, const USFPawnData*);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerInfoChangedDelegate, const FSFPlayerSelectionInfo&, NewPlayerSelectionInfo);

/** Defines the types of client connected */
UENUM()
enum class ESFPlayerConnectionType : uint8
{
	// An active player
	Player = 0,

	// Spectator connected to a running game
	LiveSpectator,

	// Spectating a demo recording offline
	ReplaySpectator,

	// A deactivated player (disconnected)
	InactivePlayer
};

UCLASS()
class SF_API ASFPlayerState : public APlayerState, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
	
public:
	ASFPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostNetInit() override;

	//~ IGenericTeamAgentInterface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~ End IGenericTeamAgentInterface

	UFUNCTION(BlueprintCallable, Category = "SF|PlayerState")
	ASFPlayerController* GetSFPlayerController() const;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	USFAbilitySystemComponent* GetSFAbilitySystemComponent() const;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void StartLoadingPawnData();
	bool IsPawnDataLoaded() const { return bPawnDataLoaded; }

	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }
	void SetPawnData(const USFPawnData* InPawnData);

	void SetPlayerSelection(const FSFPlayerSelectionInfo& NewPlayerSelection);
	const FSFPlayerSelectionInfo& GetPlayerSelection() const { return PlayerSelection; }

	void SetIsReadyForTravel(bool bInIsReadyForTravel);
	bool GetIsReadyForTravel() const { return bIsReadyForTravel; }

	UFUNCTION(BlueprintPure, Category = "SF|PlayerState")
	bool IsDead() const;

	//~AActor interface
	virtual void PostInitializeComponents() override;
	//~End of AActor interface

	//~APlayerState interface
	virtual void Reset() override;
	virtual void ClientInitialize(AController* C) override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OnDeactivated() override;
	virtual void OnReactivated() override;
	//~End of APlayerState interface

	void SetPlayerConnectionType(ESFPlayerConnectionType NewType);
	ESFPlayerConnectionType GetPlayerConnectionType() const { return MyPlayerConnectionType; }
	const USFPrimarySet_Hero* GetPrimarySet() const { return PrimarySet; }
	const USFCombatSet_Hero* GetCombatSet() const { return CombatSet; }

	// Gets the replicated view rotation of this player, used for spectating
	FRotator GetReplicatedViewRotation() const;

	// Sets the replicated view rotation, only valid on the server
	void SetReplicatedViewRotation(const FRotator& NewRotation);

	// 트래블 직전에 데이터를 미리 저장하는 함수
	void SavePersistedData();

	// 저장된 어빌리티 시스템 데이터가 있는지 (초기 PawnData AbilitySet 부여 스킵 판단용)
	bool HasSavedAbilitySystemData() const { return SavedASCData.IsValid(); }

	// Seamless Travel 후 ASC 데이터 복원
	void RestorePersistedAbilityData();

	// 스킬 업그레이드 요청 (클라이언트 → 서버)
	UFUNCTION(Server, Reliable)
	void Server_RequestSkillUpgrade(TSubclassOf<USFGameplayAbility> NewAbilityClass, FGameplayTag InputTag);

	//=====Permanent Upgrade=====
	// 강화 데이터(서버에서만 확정). 실 데이터는 클라이언트 PlayFab 로드 후 Server RPC로 전달됨.
	void SetPermanentUpgradeData(const FSFPermanentUpgradeData& InData);
	const FSFPermanentUpgradeData& GetPermanentUpgradeData() const { return PermanentUpgradeData; }

	// 클라이언트 → 서버 업그레이드 데이터 제출
	UFUNCTION(Server, Reliable)
	void Server_SubmitPermanentUpgradeData(const FSFPermanentUpgradeData& InData);
	//===========================

	int32 GetGold() const { return Gold; }
	void SetGold(const int32 NewGold) { Gold = NewGold; }
	void AddGold(const int32 Amount) { Gold += Amount; }

private:
	void OnPawnDataLoadComplete(const USFPawnData* LoadedPawnData);
	void ApplySkillUpgrade(TSubclassOf<USFGameplayAbility> NewAbilityClass, FGameplayTag InputTag);

	UFUNCTION()
	void OnRep_PawnData();

	UFUNCTION()
	virtual void OnRep_PlayerSelection();

	UFUNCTION()
	void OnRep_IsReadyForTravel();

	// Permanent upgrade apply helper
	void TryApplyPermanentUpgrade();
	static bool ArePermanentUpgradeDataEqual(const FSFPermanentUpgradeData& A, const FSFPermanentUpgradeData& B);
	FTimerHandle PermanentUpgradeRetryTimer;
	void SchedulePermanentUpgradeRetry();
	bool bPermanentUpgradeAppliedThisGame = false;


public:
	FOnPawnDataLoaded OnPawnDataLoaded;

	UPROPERTY(BlueprintAssignable, Category = "SF|Events")
	FOnPlayerInfoChangedDelegate OnPlayerInfoChanged;

private:
	// 어빌리티 시스템 컴포넌트에서 PawnData를 참조해서 능력을 부여하기 위해 캐싱을 해놓음
	UPROPERTY(ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const USFPawnData> PawnData;

	UPROPERTY(ReplicatedUsing = OnRep_PlayerSelection)
	FSFPlayerSelectionInfo PlayerSelection;

	UPROPERTY(ReplicatedUsing = OnRep_IsReadyForTravel)
	uint8 bIsReadyForTravel : 1;

	UPROPERTY(Replicated)
	FRotator ReplicatedViewRotation;

	UPROPERTY(VisibleAnywhere, Category = "SF|PlayerState")
	TObjectPtr<USFAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<USFPrimarySet_Hero> PrimarySet;

	UPROPERTY()
	TObjectPtr<USFCombatSet_Hero> CombatSet;

	UPROPERTY(Replicated)
	ESFPlayerConnectionType MyPlayerConnectionType;

	UPROPERTY(Replicated)
	FGenericTeamId MyTeamID = FGenericTeamId(SFTeamID::Player);

	UPROPERTY()
	uint8 bPawnDataLoaded : 1;

	TSharedPtr<FStreamableHandle> PawnDataHandle;

	// 재화
	UPROPERTY(Replicated)
	int32 Gold = 0;

	// Seamless Travel 간 ASC 데이터 저장용
	UPROPERTY()
	FSFSavedAbilitySystemData SavedASCData;

	//=====Permanent Upgrade=====
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USFPermanentUpgradeComponent> PermanentUpgradeComponent;

	// 강화 데이터 Replicate
	UPROPERTY(Replicated)
	FSFPermanentUpgradeData PermanentUpgradeData;

	// "데이터를 서버가 받았다" 플래그 (값이 0이어도 받았으면 true)
	bool bPermanentUpgradeDataReceived = false;

	// 마지막으로 적용했던 데이터 (중복 적용 방지)
	bool bHasLastAppliedPermanentUpgradeData = false;
	FSFPermanentUpgradeData LastAppliedPermanentUpgradeData;
	//===========================

	
	//=====Common Upgrade=======
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USFCommonUpgradeComponent> CommonUpgradeComponent;
	
	//=====Combat State=====
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USFPlayerCombatStateComponent> CombatStateComponent;
	

	
};
