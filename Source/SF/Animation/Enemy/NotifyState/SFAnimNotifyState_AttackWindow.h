// SFAnimNotifyState_AttackWindow.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SFAnimNotifyState_AttackWindow.generated.h"

UCLASS()
class SF_API USFAnimNotifyState_AttackWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;  // ✅ override 추가
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
	UPROPERTY(EditAnywhere, Category= CachedSocket)
	TArray<FName> AllowedSocketNames;
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> CachedWeapons;  
	
	void FindWeapons(USkeletalMeshComponent* MeshComp);
};