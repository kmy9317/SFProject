#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "SFDragonMovementStateBase.generated.h"

class USFDragonMovementComponent;
class UCharacterMovementComponent;
class ACharacter;

/**
 * 드래곤 이동 상태의 기본 클래스
 * 모든 구체적인 상태(Flying, Landing 등)는 이 클래스를 상속받아 구현
 */
UCLASS(Abstract, Blueprintable)
class SF_API USFDragonMovementStateBase : public UObject
{
	GENERATED_BODY()

public:
	// 상태 초기화 
	virtual void Initialize(class USFDragonMovementComponent* InOwnerMovement);

	// 상태 진입 시 호출 
	virtual void EnterState() {};

	// 매 프레임 업데이트 
	virtual void UpdateState(float DeltaTime){};

	// 상태 종료 시 호출 
	virtual void ExitState() {};

	// 이 상태로 전환 가능한지 체크 
	virtual bool CanTransitionTo();

	// 상태 타입 반환 (각 State에서 override 필수) - GameplayTag 기반
	virtual FGameplayTag GetType() const { return FGameplayTag(); };

protected:
	UPROPERTY()
	TObjectPtr<USFDragonMovementComponent> OwnerMovementComponent;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter;
};