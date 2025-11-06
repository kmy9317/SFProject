#pragma once

#include "CoreMinimal.h"
#include "SFPlayerInfoTypes.generated.h"

class APlayerState;
class USFHeroDefinition;

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
	FORCEINLINE const USFHeroDefinition* GetHeroDefinition() const { return HeroDefinition; }
	FORCEINLINE void SetHeroDefinition(USFHeroDefinition* NewDefinition) { HeroDefinition = NewDefinition; }

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
};