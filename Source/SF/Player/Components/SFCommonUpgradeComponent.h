#pragma once

#include "CoreMinimal.h"
#include "Components/PlayerStateComponent.h"
#include "SFCommonUpgradeComponent.generated.h"

struct FSFCommonUpgradeChoice;
class USFCommonLootTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeChoicesReceived, const TArray<FSFCommonUpgradeChoice>&, Choices);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpgradeApplied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeApplyFailed, const FText&, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRerollFailed, const FText&, Reason);

/**
 * 일반 강화 시스템 클라이언트/서버 통신 컴포넌트
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFCommonUpgradeComponent : public UPlayerStateComponent
{
	GENERATED_BODY()

public:
	USFCommonUpgradeComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|Upgrade")
	void RequestGenerateChoices(USFCommonLootTable* LootTable, int32 StageIndex, int32 Count = 3);

	UFUNCTION(BlueprintCallable, Category = "SF|Upgrade")
	void RequestApplyUpgrade(const FGuid& ChoiceId);

	UFUNCTION(BlueprintCallable, Category = "SF|Upgrade")
	void RequestApplyUpgradeByIndex(int32 ChoiceIndex);

	UFUNCTION(BlueprintCallable, Category = "SF|Upgrade")
	void RequestReroll();

	// 현재 대기 중인 선택지
	UFUNCTION(BlueprintPure, Category = "SF|Upgrade")
	const TArray<FSFCommonUpgradeChoice>& GetPendingChoices() const { return PendingChoices; }

	// 선택 대기 중인지 
	UFUNCTION(BlueprintPure, Category = "SF|Upgrade")
	bool HasPendingChoices() const { return !PendingChoices.IsEmpty(); }

	// 리롤 가능 여부 
	UFUNCTION(BlueprintPure, Category = "SF|Upgrade")
	bool CanReroll() const;

	// 추가 선택 가능 여부 (특정 태그 보유 시)
	UFUNCTION(BlueprintPure, Category = "SF|Upgrade")
	bool HasExtraSelectionTag() const;

	UFUNCTION(BlueprintCallable, Category = "SF|Upgrade")
	void ClearPendingChoices();

	UFUNCTION(BlueprintPure, Category = "SF|Upgrade")
	bool IsPendingExtraSelection() const { return bPendingExtraSelection; }

protected:

	// 서버 → 클라이언트: 선택지 전송
	UFUNCTION(Client, Reliable)
	void Client_ReceiveUpgradeChoices(const TArray<FSFCommonUpgradeChoice>& Choices, bool bIsExtraSelection);

	// 서버 → 클라이언트: 적용 완료 알림 
	UFUNCTION(Client, Reliable)
	void Client_NotifyUpgradeApplied();

	// 서버 → 클라이언트: 적용 실패 알림 
	UFUNCTION(Client, Reliable)
	void Client_NotifyUpgradeApplyFailed(const FText& Reason);
	
	// 서버 → 클라이언트: 리롤 실패 알림
	UFUNCTION(Client, Reliable)
	void Client_NotifyRerollFailed(const FText& Reason);

	// 클라이언트 → 서버: 선택 적용 요청
	UFUNCTION(Server, Reliable)
	void Server_RequestApplyUpgrade(const FGuid& ChoiceId);

	// 클라이언트 → 서버: 리롤 요청 
	UFUNCTION(Server, Reliable)
	void Server_RequestReroll();

private:
	void ConsumeExtraSelectionTag();

public:

	UPROPERTY(BlueprintAssignable, Category = "SF|Upgrade")
	FOnUpgradeChoicesReceived OnChoicesReceived;

	UPROPERTY(BlueprintAssignable, Category = "SF|Upgrade")
	FOnUpgradeApplied OnUpgradeApplied;

	UPROPERTY(BlueprintAssignable, Category = "SF|Upgrade")
	FOnUpgradeApplyFailed OnUpgradeApplyFailed;

	UPROPERTY(BlueprintAssignable, Category = "SF|Upgrade")
	FOnRerollFailed OnRerollFailed;

private:
	// 클라이언트에서 캐싱된 선택지 (UI 표시용)
	UPROPERTY()
	TArray<FSFCommonUpgradeChoice> PendingChoices;

	// 현재 스테이지 (리롤 시 재사용)
	UPROPERTY()
	int32 CachedStageIndex = 0;

	// 캐싱된 루트 테이블 (추가 선택 시 재사용)
	UPROPERTY()
	TObjectPtr<USFCommonLootTable> CachedLootTable;

	// 선택지 개수 (추가 선택 시 재사용)
	UPROPERTY()
	int32 CachedChoiceCount = 3;

	bool bPendingExtraSelection = false;
};
