#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayEffectTypes.h"
#include "Character/SFCharacterBase.h"
#include "SFHeroAnimInstance.generated.h"

class UCharacterMovementComponent;
class UAbilitySystemComponent;
class ASFCharacterBase; // 전방 선언 추가

/**
 * 
 */
UCLASS()
class SF_API USFHeroAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	USFHeroAnimInstance();

	// 초기화 함수 (BeginPlay와 유사)
	virtual void NativeInitializeAnimation() override;

	// 매 프레임 호출되지만, 게임 스레드가 아닌 워커 스레드에서도 안전하게 호출됨 (핵심!)
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	// AbilitySystemComponent 초기화
	virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC);

protected:
	// === References ===
	// 약한 참조(Weak Object Ptr)를 사용하여 크래시 방지 및 스레드 안전성 확보
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "References")
	TObjectPtr<ASFCharacterBase> OwnerCharacter; // ACharacter에서 ASFCharacterBase로 변경

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "References")
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	// === Anim Graph Variables (Thread Safe) ===
	// 이 변수들은 AnimGraph에서 'Fast Path'로 접근 가능해야 함

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Data")
	FVector Velocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Data")
	float GroundSpeed;			// 지면 이동 속력

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Data")
	float LocomotionDirection;	// -180 ~ 180 이동 방향

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Data")
	bool bShouldMove;			// 가속도가 있는지 (입력 여부)

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Data")
	bool bIsFalling;			// 공중 체류 여부

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Data")
	bool bIsCrouching;			// 앉기 여부

	// === Gameplay Tags ===
	// Gameplay tags that can be mapped to blueprint variables. The variables will automatically update as the tags are added or removed.
	UPROPERTY(EditDefaultsOnly, Category = "GameplayTags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;

	// === Combat State (Thread Safe) ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State")
	bool bIsAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State")
	bool bIsBlocking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State")
	bool bIsStunned = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|State")
	bool bIsSprinting = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Data")
	float LastMoveSpeed;
};
