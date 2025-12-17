// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "SFAnimNotifyState_GrantTag.generated.h"

/**
 * 
 */
UCLASS(meta=(DisplayName="SF Grant Gameplay Tag"))
class SF_API USFAnimNotifyState_GrantTag : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	// 에디터에서 설정 가능: 이 노티파이 구간 동안 캐릭터에게 부여할 태그
	// 반드시 'EditAnywhere'여야 몽타주 디테일 패널에서 설정 가능
	UPROPERTY(EditAnywhere, Category = "Gameplay Tags")
	FGameplayTag GameplayTag;

	// 종료 이벤트용 별도 태그 (비어있으면 GameplayTag 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (EditCondition = "bBroadcastEventOnEnd"))
	FGameplayTag EndEventTag;

	// 시작 시 GameplayEvent 전송
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	bool bBroadcastEventOnBegin = false;

	// 종료 시 GameplayEvent 전송 (Commit 시점 알림 등에 유용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	bool bBroadcastEventOnEnd = false;

	// 몽타주가 이 노티파이에 진입했을 때 호출
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	// 몽타주가 이 노티파이에서 벗어났을 때 호출
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
	void BroadcastGameplayEvent(UAbilitySystemComponent* ASC, AActor* Owner, const FGameplayTag& EventTag);
};
