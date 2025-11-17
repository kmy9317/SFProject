// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Controller/SFWidgetController.h"

#include "SFPartyMemberWidgetController.generated.h"

struct FSFPlayerSelectionInfo;
class USFUserWidget;

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class SF_API USFPartyMemberWidgetController : public USFWidgetController
{
	GENERATED_BODY()
public:
	// 이 컨트롤러는 다른 클라이언트들의 PlayerState를 기반으로 데이터를 바인딩
	virtual void BindCallbacksToDependencies() override;
    
	// 이 컨트롤러는 다른 클라이언트들의 PlayerState의 현재 값을 브로드캐스트
	virtual void BroadcastInitialSets() override;

public:
	// WBP_PartyMemberEntry 위젯이 바인딩할 델리게이트
	UPROPERTY(BlueprintAssignable, Category="SF|Attributes")
	FOnAttributeChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="SF|Attributes")
	FOnAttributeChangedSignature OnMaxHealthChanged;
    
	// (필요에 따라 Mana, Stamina 등 추가)

	// FSFPlayerSelectionInfo 구조체 변경을 알릴 델리게이트
	UPROPERTY(BlueprintAssignable, Category="SF|Info")
	FOnPlayerInfoChangedSignature OnPlayerInfoChanged;

protected:
	// 이 파티원의 PlayerState에 있는 OnRep_PlayerSelectionInfo가 호출될 때 실행될 핸들러
	UFUNCTION()
	void HandlePlayerInfoChanged(const FSFPlayerSelectionInfo& PlayerInfo);


};
