// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Controller/SFWidgetController.h"
#include "SFPartyWidgetController.generated.h"

class USFUserWidget;
class USFPartyMemberWidgetController;
class ASFPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartyMemberControllerChanged, USFPartyMemberWidgetController*, MemberController);

/**
 * 로컬 플레이어에서 관리하는 파티 멤버 UI 컨트롤러를 관리하는 매니저 컨트롤러
 */
UCLASS(BlueprintType, Blueprintable)
class SF_API USFPartyWidgetController : public USFWidgetController
{
	GENERATED_BODY()

public:
	/** GameState->PlayerArray를 순회하며 OnPartyMemberAdded를 브로드캐스트 */
	virtual void BroadcastInitialSets() override;
	
	// 파티 관리를 위해 GameState에 바인딩
	virtual void BindCallbacksToDependencies() override;

	// 이 컨트롤러가 소유한 개별 멤버 컨트롤러를 반환
	UFUNCTION(BlueprintPure, Category="SF|Party")
	USFPartyMemberWidgetController* GetMemberController(APlayerState* PlayerState) const;

protected:
	// GameState의 C++ 델리게이트에 바인딩될 내부 핸들러
	UFUNCTION()
	void HandlePlayerAdded(APlayerState* InPlayerState);

	UFUNCTION()
	void HandlePlayerRemoved(APlayerState* InPlayerState);

	// 멤버 컨트롤러를 생성하고 브로드캐스트
	void AddAndBroadcastMember(ASFPlayerState* PlayerState);

	// 멤버 컨트롤러를 제거하고 브로드캐스트
	void RemoveAndBroadcastMember(ASFPlayerState* PlayerState);

public:
	// UMG(WBP_PartyUI)가 바인딩할 델리게이트
	UPROPERTY(BlueprintAssignable, Category="SF|Party")
	FOnPartyMemberControllerChanged OnPartyMemberAdded;

	UPROPERTY(BlueprintAssignable, Category="SF|Party")
	FOnPartyMemberControllerChanged OnPartyMemberRemoved;

	// 이 컨트롤러가 개별 멤버 컨트롤러를 소유/관리
	TMap<TWeakObjectPtr<APlayerState>, TObjectPtr<USFPartyMemberWidgetController>> PartyMemberControllerMap;
	
private:
	// 파티 멤버 위젯 컨트롤러의 블루프린트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "SF|Controller")
	TSubclassOf<USFPartyMemberWidgetController> PartyMemberWidgetControllerClass;
};
