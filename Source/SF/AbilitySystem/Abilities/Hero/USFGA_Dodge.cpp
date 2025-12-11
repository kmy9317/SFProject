// Fill out your copyright notice in the Description page of Project Settings.


#include "USFGA_Dodge.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/SFCharacterBase.h"
#include "MotionWarpingComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "GameFramework/CharacterMovementComponent.h"

USFGA_Dodge::USFGA_Dodge(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilityTags.AddTag(SFGameplayTags::Ability_Hero_Dodge);
}

void USFGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (!Character)
	{
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
		return;
	}

	// 1. 공중 사용 불가
	if (Character->GetCharacterMovement()->IsFalling())
	{
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
		return;
	}

	// 2. 비용(스태미나) 지불
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
		return;
	}

	// 3. 구를 방향 및 위치 계산
	FVector TargetLocation;
	FRotator TargetRotation;
	CalculateDodgeParameters(TargetLocation, TargetRotation);

	// 4. Motion Warping 타겟 설정
	SetupMotionWarping(TargetLocation, TargetRotation);

	// 5. 캐릭터 회전 (구르는 방향을 바라보게 함)
	Character->SetActorRotation(TargetRotation);

	// 6. 몽타주 재생
	if (DodgeMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			TEXT("DodgeMontage"),
			DodgeMontage,
			1.f,
			NAME_None,
			true // StopWhenAbilityEnds
		);

		if (MontageTask)
		{
			MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
			MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
			MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
			MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageFinished);
			MontageTask->ReadyForActivation();
		}
	}
    else
    {
        // 몽타주가 없으면 즉시 종료
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

void USFGA_Dodge::CalculateDodgeParameters(FVector& OutLocation, FRotator& OutRotation) const
{
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (!Character) return;

	// 플레이어의 입력 방향 가져오기 (WASD)
	FVector InputDir = Character->GetLastMovementInputVector();

	// 입력이 없으면 캐릭터의 반대 방향(백스탭) 혹은 전방(그냥 구르기) 설정
	// 소울류: 입력 없으면 '백스텝'이 국룰이지만, 취향에 따라 전방으로 해도 됨.
    // 여기서는 '입력 없으면 백스텝(-Forward)'으로 구현
	if (InputDir.IsNearlyZero())
	{
		InputDir = -Character->GetActorForwardVector();
	}
	else
	{
		InputDir.Normalize();
	}

	OutRotation = InputDir.Rotation();

	// 목표 위치 계산 (현재 위치 + 방향 * 거리)
	FVector StartParams = Character->GetActorLocation();
	FVector EndParams = StartParams + (InputDir * DodgeDistance);

	// **벽 뚫기 방지 (LineTrace)**
	FHitResult HitResult;
    // 채널은 프로젝트 설정에 맞게 변경 (예: ECC_WorldStatic)
	GetWorld()->LineTraceSingleByChannel(HitResult, StartParams, EndParams, ECC_Visibility);

	if (HitResult.bBlockingHit)
	{
		// 벽에 닿았으면, 벽보다 살짝 앞(30유닛)까지만 이동
		OutLocation = HitResult.Location - (InputDir * 30.f);
	}
	else
	{
		OutLocation = EndParams;
	}
}

void USFGA_Dodge::SetupMotionWarping(const FVector& TargetLocation, const FRotator& TargetRotation)
{
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (Character)
	{
		if (UMotionWarpingComponent* MW = Character->GetMotionWarpingComponent())
		{
			// 위치와 회전을 모두 타겟팅
			MW->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, TargetRotation);
		}
	}
}

void USFGA_Dodge::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Motion Warping 타겟 정리
	if (ASFCharacterBase* Character = GetSFCharacterFromActorInfo())
	{
		if (UMotionWarpingComponent* MW = Character->GetMotionWarpingComponent())
		{
			MW->RemoveWarpTarget(WarpTargetName);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}