#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayEffect.h"
#include "SFGE_InternalInvincibility.generated.h"

// [내부용] 방어력을 무한대로 올려주는 C++ 전용 GameplayEffect
// 별도의 블루프린트 에셋 없이 C++ 코드만으로 동작하게 해줍니다.
UCLASS()
class USFGE_InternalInvincibility : public UGameplayEffect
{
	GENERATED_BODY()
public:
	USFGE_InternalInvincibility();
};

/**
 * 몽타주 구간 동안 무적(방어력 극대화)을 부여하는 노티파이
 */
UCLASS(meta = (DisplayName = "SF Invincibility (C++ Only)"))
class SF_API USFAnimNotifyState_Invincible : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
	// 적용된 이펙트를 추적하기 위한 핸들 맵 (여러 액터가 동시에 사용할 경우 대비)
	UPROPERTY()
	TMap<USkeletalMeshComponent*, FActiveGameplayEffectHandle> ActiveEffectHandles;
};