// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Interact.generated.h"

struct FSFInteractionInfo;
/**
 * 기본 상호작용 어빌리티 클래스
 * - 주변 상호작용 가능한 객체 감지 (구체 범위 스캔)
 * - 시선 방향 정밀 타겟팅 (레이캐스트)
 * - 상호작용 입력 대기 및 처리
 * - 홀딩 상호작용 어빌리티 트리거
 * - UI 업데이트 메시지 전송
 */
UCLASS()
class SF_API USFGA_Interact : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Interact(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	
	// 감지된 상호작용 정보들을 업데이트하고 UI에 알림(레이캐스트와 구체 스캔 결과를 받아서 현재 상호작용 정보를 갱신)
	UFUNCTION(BlueprintCallable)
	void UpdateInteractions(const TArray<FSFInteractionInfo>& InteractionInfos);

	// 상호작용을 실제로 트리거(Active Interaction Ability 활성화)
	UFUNCTION(BlueprintCallable)
	void TriggerInteraction();

private:
	// 상호작용 입력 대기를 시작
	void WaitInputStart();

	UFUNCTION()
	void OnInputStart();

protected:
	// 현재 감지된 상호작용 정보들 (UI 업데이트 및 트리거에 사용)
	UPROPERTY(BlueprintReadWrite)
	TArray<FSFInteractionInfo> CurrentInteractionInfos;

	// 시선 방향 레이캐스트의 최대 거리 (cm 단위)
	UPROPERTY(EditDefaultsOnly, Category="SF|Interaction")
	float InteractionTraceRange = 150.f;

	// Task의 시선 방향 레이캐스트 수행 주기 (초 단위)
	UPROPERTY(EditDefaultsOnly, Category="SF|Interaction")
	float InteractionTraceRate = 0.1f;

	// 플레이어 주변에서 상호작용 가능한 객체를 찾는 범위
	UPROPERTY(EditDefaultsOnly, Category="SF|Interaction")
	float InteractionScanRange = 500.f;

	// Task의 주변 객체 구체 스캔 수행 주기 (초 단위)
	UPROPERTY(EditDefaultsOnly, Category="SF|Interaction")
	float InteractionScanRate = 0.1f;

	// 상호작용 감지 레이캐스트 디버그 라인 표시 여부
	UPROPERTY(EditDefaultsOnly, Category="SF|Interaction")
	bool bShowTraceDebug = false;

	// 기본 상호작용 UI 위젯 클래스 (FSFInteractionInfo에 커스텀 위젯이 지정되지 않은 경우 사용)
	UPROPERTY(EditDefaultsOnly, Category="SF|Interaction")
	TSoftClassPtr<UUserWidget> DefaultInteractionWidgetClass;
};
