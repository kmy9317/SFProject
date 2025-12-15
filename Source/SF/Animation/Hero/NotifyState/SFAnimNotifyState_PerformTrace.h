// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SFAnimNotifyState_PerformTrace.generated.h"

class ASFEquipmentBase;

UENUM(BlueprintType)
enum class ESFTraceMethod : uint8
{
	// 컴포넌트 콜리전 사용 (일반 평타, 휘어진 무기 등 복잡한 형태)
	ComponentSweep,
	// 박스 형태 사용 (스킬에 따라 크기 조절 가능)
	BoxSweep
};

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

	// 트레이스 방식 선택
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESFTraceMethod TraceMethod = ESFTraceMethod::BoxSweep;

	// 감지할 오브젝트 타입들
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
};

// BoxSweep 방식에서만 사용
USTRUCT(BlueprintType)
struct FTraceShapeParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseWeaponDefaultExtent = true;

	// 무기에 TraceBox 없는경우 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bUseWeaponDefaultExtent"))
	FVector BoxExtent = FVector(50.f, 5.f, 5.f);

	// X: 무기 길이, Y: 무기 두께, Z: 무기 너비
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUseWeaponDefaultExtent"))
	FVector ExtentScale = FVector::OneVector;

	// 박스 중심 오프셋 (로컬 좌표)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector PivotOffset = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FTraceDebugParams
{
	GENERATED_BODY()

	// 무기 Pivot 설정을 위한 로그 출력
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLogTraceInfo = false;
	
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

private:
	FVector GetTraceBoxExtent() const;

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
	FTraceShapeParams TraceShapeParams;

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
