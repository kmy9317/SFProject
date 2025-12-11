// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "USFGA_Dodge.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Dodge : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Dodge(FObjectInitializer const& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 몽타주 종료 콜백
	UFUNCTION()
	void OnMontageFinished();

	// 구르기 계산 로직
	void CalculateDodgeParameters(FVector& OutLocation, FRotator& OutRotation) const;
	void SetupMotionWarping(const FVector& TargetLocation, const FRotator& TargetRotation);

protected:
	// 구르기 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "SF|Animation")
	TObjectPtr<UAnimMontage> DodgeMontage;

	// 구르기 이동 거리
	UPROPERTY(EditDefaultsOnly, Category = "SF|Dodge")
	float DodgeDistance = 500.f;

	// Motion Warping 타겟 이름 (몽타주 노티파이와 일치해야 함)
	UPROPERTY(EditDefaultsOnly, Category = "SF|MotionWarping")
	FName WarpTargetName = TEXT("Dodge");
};