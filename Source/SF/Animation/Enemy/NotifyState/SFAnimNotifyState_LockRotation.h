#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SFAnimNotifyState_LockRotation.generated.h"

/**
 * 공격 도중 AI가 회전하지 못하도록 강제 잠금 (회피 가능하도록)
 */
UCLASS()
class SF_API USFAnimNotifyState_LockRotation : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	USFAnimNotifyState_LockRotation();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};