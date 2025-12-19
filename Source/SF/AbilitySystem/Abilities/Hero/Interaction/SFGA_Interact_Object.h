#pragma once

#include "CoreMinimal.h"
#include "SFGA_Interact_Info.h"
#include "SFGA_Interact_Object.generated.h"

/**
 * 오브젝트 상호작용을 위한 기본 어빌리티 클래스
 * - 상호작용 유효성 검증 및 권한 확인
 * - 위치/각도 기반 상호작용 취소 모니터링
 */
UCLASS()
class SF_API USFGA_Interact_Object : public USFGA_Interact_Info
{
	GENERATED_BODY()

public:
	USFGA_Interact_Object(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
protected:
	
	// 플레이어가 허용 범위를 벗어났을 때 어빌리티를 취소
	UFUNCTION()
	void OnInvalidInteraction();

protected:

	// 플레이어가 상호작용 대상을 바라보는 각도의 허용 범위 (도 단위)
	UPROPERTY(EditDefaultsOnly, Category="SF|Interaction")
	float AcceptanceAngle = 90.f;

	// 플레이어와 상호작용 대상 간의 최대 허용 거리 (cm 단위)
	UPROPERTY(EditDefaultsOnly, Category="SF|Interaction")
	float AcceptanceDistance = 100.f;
	
protected:
	
	// 어빌리티가 성공적으로 초기화되었는지 여부 
	bool bInitialized = false;
};
