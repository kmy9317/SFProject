#pragma once

#include "CoreMinimal.h"
#include "SFPlayerInfoTypes.generated.h"

class APlayerState;
class USFHeroDefinition;

USTRUCT(BlueprintType)
struct FSFPlayerInfo
{
	GENERATED_BODY()

public:
	FSFPlayerInfo()
		: PC(nullptr)
		, PS(nullptr)
		, PlayerName(TEXT(""))
		, bReady(false)
	{}

	FSFPlayerInfo(APlayerController* InPC, APlayerState* InPS, const FString& InPlayerName, bool bInReady)
		: PC(InPC)
		, PS(InPS)
		, PlayerName(InPlayerName)
		, bReady(bInReady)
	{}

	UPROPERTY(BlueprintReadOnly, NotReplicated)
	TObjectPtr<APlayerController> PC;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APlayerState> PS;

	UPROPERTY(BlueprintReadOnly)
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly)
	bool bReady;

	/** 유효성 검사 */
	bool IsValid() const
	{
		return PC != nullptr && PS != nullptr;
	}

	/** 비교 연산자 (RepNotify 비교용) */
	bool operator==(const FSFPlayerInfo& Other) const
	{
		return PC == Other.PC && 
			   PS == Other.PS && 
			   PlayerName == Other.PlayerName && 
			   bReady == Other.bReady;
	}

	bool operator!=(const FSFPlayerInfo& Other) const
	{
		return !(*this == Other);
	}
};

USTRUCT()
struct FSFPlayerSelectionInfo
{
	GENERATED_BODY()
public:
	FSFPlayerSelectionInfo() ;
	FSFPlayerSelectionInfo(uint8 InSlot, const APlayerState* InPlayerState);

	FORCEINLINE void SetSlot(uint8 NewSlot) { Slot = NewSlot; }
	FORCEINLINE uint8 GetPlayerSlot() const { return Slot; }
	FORCEINLINE FUniqueNetIdRepl GetPlayerUniqueId() const { return PlayerUniqueId; }
	FORCEINLINE FString GetPlayerNickname() const { return PlayerNickname; }
	FORCEINLINE USFHeroDefinition* GetHeroDefinition() const { return HeroDefinition; }
	FORCEINLINE void SetHeroDefinition(USFHeroDefinition* NewDefinition) { HeroDefinition = NewDefinition; }
	FORCEINLINE bool IsReady() const { return bReady; }
	FORCEINLINE void SetReady(bool bInReady) { bReady = bInReady; }

	bool IsForPlayer(const APlayerState* PlayerState) const;
	bool IsValid() const;
	static uint8 GetInvalidSlot();
	
private:
	UPROPERTY()
	uint8 Slot;

	UPROPERTY()
	FUniqueNetIdRepl PlayerUniqueId;

	UPROPERTY()
	FString PlayerNickname;

	UPROPERTY()
	TObjectPtr<USFHeroDefinition> HeroDefinition;

	UPROPERTY()
	bool bReady;
};