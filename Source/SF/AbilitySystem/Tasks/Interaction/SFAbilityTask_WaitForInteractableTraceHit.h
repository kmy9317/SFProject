#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Interaction/SFInteractionQuery.h"
#include "SFAbilityTask_WaitForInteractableTraceHit.generated.h"

struct FSFInteractionInfo;
class ISFInteractable;

// 상호작용 감지 변화 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableChanged, const TArray<FSFInteractionInfo>&, InteractableInfos);

/**
 * 플레이어의 시선 방향으로 주기적으로 레이캐스트를 수행하여
 * 상호작용 가능한 객체들을 감지하고 추적하는 어빌리티 태스크
 */
UCLASS()
class SF_API USFAbilityTask_WaitForInteractableTraceHit : public UAbilityTask
{
	GENERATED_BODY()

public:
	USFAbilityTask_WaitForInteractableTraceHit(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta=(HidePin="OwningAbility", DefaultToSelf="OwningAbility", BlueprintInternalUseOnly="true"))
	static USFAbilityTask_WaitForInteractableTraceHit* WaitForInteractableTraceHit(UGameplayAbility* OwningAbility, FSFInteractionQuery InteractionQuery, ECollisionChannel TraceChannel, FGameplayAbilityTargetingLocationInfo StartLocation, float InteractionTraceRange = 100.f, float InteractionTraceRate = 0.1f, bool bShowDebug = false);

protected:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void PerformTrace();

	// 플레이어 컨트롤러의 카메라 시점을 기반으로 레이캐스트 종료점을 계산(카메라 방향과 플레이어 위치 기준 구체 범위를 고려한 타겟팅)
	void AimWithPlayerController(const AActor* InSourceActor, const FCollisionQueryParams& Params, const FVector& TraceStart, float MaxRange, FVector& OutTraceEnd, bool bIgnorePitch = false) const;

	// 카메라 레이를 플레이어 중심의 구체 범위 내로 제한하는 함수(카메라가 플레이어에서 멀리 떨어져 있어도 상호작용 범위를 벗어나지 않도록 보장)
	bool ClipCameraRayToAbilityRange(const FVector& CameraLocation, const FVector& CameraDirection, const FVector& AbilityCenter, const float AbilityRange, FVector& OutClippedPosition) const;

	void LineTrace(const FVector& Start, const FVector& End, const FCollisionQueryParams& Params, FHitResult& OutHitResult) const;
	
	// 감지된 상호작용 가능한 객체들로부터 상호작용 정보를 업데이트 및 이전 정보와 비교하여 변화가 있을 때만 델리게이트를 호출
	void UpdateInteractionInfos(const FSFInteractionQuery& InteractQuery, const TArray<TScriptInterface<ISFInteractable>>& Interactables);

	// 상호작용 가능한 객체들의 하이라이트 효과를 관리
	void HighlightInteractables(const TArray<FSFInteractionInfo>& InteractionInfos, bool bShouldHighlight);

public:
	UPROPERTY(BlueprintAssignable)
	FOnInteractableChanged InteractableChanged;

private:

	// 요청자를 포함한 상호작용 쿼리
	UPROPERTY()
	FSFInteractionQuery InteractionQuery;

	// 레이캐스트 시작 위치 정
	UPROPERTY()
	FGameplayAbilityTargetingLocationInfo StartLocation;

	// 레이캐스트에 사용할 콜리전 채널 
	ECollisionChannel TraceChannel = ECC_Visibility;

	float InteractionTraceRange = 100.f;
	float InteractionTraceRate = 0.1f;
	bool bShowDebug = false;
	bool bIsLocalPlayer = false;

	FTimerHandle TraceTimerHandle;

	// 현재 감지된 상호작용 정보들 (변화 감지용)
	TArray<FSFInteractionInfo> CurrentInteractionInfos;
};
