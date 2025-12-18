// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SFAnimNotifyState_SweepTrace.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class ESFSweepTraceType : uint8
{
	Line,
	Sphere,
	Capsule
};

USTRUCT(BlueprintType)
struct FSFSweepSocketNode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Trace")
	FName SocketName;

	// 이 소켓 지점의 반경
	UPROPERTY(EditAnywhere, Category = "Trace")
	float Radius = 30.f;
};


USTRUCT(BlueprintType)
struct FSFSweepSocketChain
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, Category = "Trace")
	TArray<FSFSweepSocketNode> Nodes;

	UPROPERTY(EditAnywhere, Category = "Trace")
	ESFSweepTraceType TraceType = ESFSweepTraceType::Capsule;
	
	UPROPERTY(EditAnywhere, Category = "Trace")
	float HalfHeightPadding = 0.f;
};





UCLASS()
class SF_API USFAnimNotifyState_SweepTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:

	UPROPERTY(EditAnywhere, Category= "SweepTrace")
	TArray<FSFSweepSocketChain> SocketChains;
	
	UPROPERTY(EditAnywhere, Category = "SweepTrace")
	FGameplayTag EventTag;

	UPROPERTY(EDitAnywhere, Category = "SweepTrace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	UPROPERTY(EditAnywhere, Category = "SweepTrace")
	bool bIsDebug = false;

	UPROPERTY(EditAnywhere, Category = "SweepTrace")
	float TraceInterval = 0.03f;

private:
	// 맞은 액터들
	TSet<TWeakObjectPtr<AActor>> HitActors;

	// 마지막 Trace 수행 시간
	float TimeSinceLastTrace = 0.f;

};
