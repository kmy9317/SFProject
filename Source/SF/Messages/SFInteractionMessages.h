#pragma once

#include "CoreMinimal.h"
#include "Interaction/SFInteractionInfo.h"
#include "SFInteractionMessages.generated.h"

/**
 * UI 업데이트를 위한 상호작용 메시지 구조체
 * UGameplayMessageSubsystem을 통해 UI에 상호작용 상태 변화를 알리는 데 사용
 */
USTRUCT(BlueprintType)
struct FSFInteractionMessage
{
	GENERATED_BODY()

public:
	// 상호작용을 요청한 액터 (보통 플레이어) 
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> Instigator = nullptr;

	// UI를 새로고침 여부
	UPROPERTY(BlueprintReadWrite)
	bool bShouldRefresh = false;

	// 상호작용 활성 상태 유무
	UPROPERTY(BlueprintReadWrite)
	bool bSwitchActive = false;

	// 현재 상호작용 정보 (제목, 설명, 지속시간 등)
	UPROPERTY(BlueprintReadWrite)
	FSFInteractionInfo InteractionInfo = FSFInteractionInfo();
};