#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "SFPlayerInfoTypes.h"
#include "GameFramework/PlayerState.h"
#include "SFPlayerState.generated.h"

class USFCombatSet_Hero;
class USFPrimarySet_Hero;
struct FStreamableHandle;
class ASFPlayerController;
class USFPawnData;
class USFAbilitySystemComponent;

// PawnData 로드 완료 델리게이트
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPawnDataLoaded, const USFPawnData*);

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

/**
 * 
 */
UCLASS()
class SF_API ASFPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	ASFPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	void SetPlayerSelection(const FSFPlayerSelectionInfo& NewPlayerSelection) { PlayerSelection = NewPlayerSelection; }
	const FSFPlayerSelectionInfo& GetPlayerSelection() const { return PlayerSelection; }

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

private:
	void OnPawnDataLoadComplete(const USFPawnData* LoadedPawnData);

public:
	FOnPawnDataLoaded OnPawnDataLoaded;
	
private:
	
	// 어빌리티 시스템 컴포넌트에서 PawnData를 참조해서 능력을 부여하기 위해 캐싱을 해놓음
	UPROPERTY(Replicated)
	TObjectPtr<const USFPawnData> PawnData;

	UPROPERTY(VisibleAnywhere, Category = "SF|PlayerState")
	TObjectPtr<USFAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const USFPrimarySet_Hero> PrimarySet;

	UPROPERTY()
	TObjectPtr<const USFCombatSet_Hero> CombatSet;

	UPROPERTY(Replicated)
	ESFPlayerConnectionType MyPlayerConnectionType;
	
	FSFPlayerSelectionInfo PlayerSelection;

	UPROPERTY()
	uint8 bPawnDataLoaded : 1;
    
	TSharedPtr<FStreamableHandle> PawnDataHandle;
};
