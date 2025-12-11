// SFNS_ParryWindow.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "SFNS_ParryWindow.generated.h"

UCLASS()
class SF_API USFNS_ParryWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	// UE5 최신 시그니처
	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	// 애님 노티파이 디테일 패널에서 선택할 패링 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SF|Parry")
	FGameplayTag ParryWindowTag;
};
