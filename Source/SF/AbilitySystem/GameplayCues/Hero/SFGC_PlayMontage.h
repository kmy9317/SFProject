#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "SFGC_PlayMontage.generated.h"

/**
 * 단순히 게임 로직에 상관없는 연출용 Montage 재생을 위한 Cue
 */
UCLASS()
class SF_API USFGC_PlayMontage : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	USFGC_PlayMontage();
	
	virtual bool HandlesEvent(EGameplayCueEvent::Type EventType) const override;
	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;
};