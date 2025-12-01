// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SFAnimNotifyState_PerformTrace.generated.h"

class ASFEquipmentBase;

USTRUCT(BlueprintType)
struct FTraceParams
{
	GENERATED_BODY()

	// 무기가 이 거리 이상 이동하면 추가 서브스텝을 생성하여 트레이스 정확도 향상
	UPROPERTY(EditAnywhere)
	float TargetDistance = 20.f;
	
	// 트레이스 기준점이 되는 소켓으로써 해당 소켓의 이동 거리를 기준으로 서브스텝 계산
	UPROPERTY(EditAnywhere)
	FName TraceSocketName = "TraceSocket";
};

USTRUCT(BlueprintType)
struct FTraceDebugParams
{
	GENERATED_BODY()

	// 디버그 그리기 활성화 여부 
	UPROPERTY(EditAnywhere)
	bool bDrawDebugShape = false;

	// 트레이스 경로 색상
	UPROPERTY(EditAnywhere)
	FColor TraceColor = FColor::Red;

	// 히트 발생 시 색상
	UPROPERTY(EditAnywhere)
	FColor HitColor = FColor::Green;
};

UCLASS()
class SF_API USFAnimNotifyState_PerformTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	USFAnimNotifyState_PerformTrace(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComponent, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComponent, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComponent, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	void PerformTrace(USkeletalMeshComponent* MeshComponent);

public:

	// EquipComponent에서 해당 슬롯에 장착된 무기를 가져옴
	UPROPERTY(EditAnywhere)
	FGameplayTag WeaponSlotTag;

	UPROPERTY(EditAnywhere)
	TEnumAsByte<ENetRole> ExecuteNetRole = ROLE_Authority;

	// 히트 발생 시 전송할 GameplayEvent 태그
	UPROPERTY(EditAnywhere)
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere)
	FTraceParams TraceParams;

	UPROPERTY(EditAnywhere)
	FTraceDebugParams TraceDebugParams;

private:

	// 특정 슬롯에 해당하는 무기 엑터 캐싱
	UPROPERTY()
	TWeakObjectPtr<ASFEquipmentBase> CachedWeaponActor;

	UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> CachedHitActors;

	// 이전 프레임의 무기 메시 Transform 
	FTransform PreviousTraceTransform;

	// 이전 프레임의 디버그 콜리전 Transform (디버그 시각화용)
	FTransform PreviousDebugTransform;

	// 이전 프레임의 소켓 Transform (서브스텝 계산용)
	FTransform PreviousSocketTransform;
};
