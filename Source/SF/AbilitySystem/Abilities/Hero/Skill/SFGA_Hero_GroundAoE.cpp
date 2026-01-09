#include "SFGA_Hero_GroundAoE.h"
#include "AbilitySystem/Abilities/Hero/Skill/SkillActor/SFGroundAOE.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h" // [추가] 필수 헤더
#include "GameFramework/PlayerController.h"
#include "Character/SFCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

USFGA_Hero_GroundAoE::USFGA_Hero_GroundAoE(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void USFGA_Hero_GroundAoE::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitCheck(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 준비 단계 시작 (Loop 몽타주 재생)
	if (AimingLoopMontage)
	{
		// PlayAnimMontage 직접 호출 대신 Task 사용
		AimingMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		   this,
		   NAME_None,
		   AimingLoopMontage,
		   1.0f,
		   NAME_None,
		   false // StopWhenAbilityEnds (수동으로 끌 것이므로 false가 낫습니다, true여도 EndAbility에서 꺼지긴 함)
		);
		// 루프 몽타주이므로 별도의 종료 델리게이트 연결 없이 실행만 시킴
		AimingMontageTask->ReadyForActivation();
	}

	// 2. 카메라 모드 변경
	if (AimingCameraModeTag.IsValid())
	{
		// 예: Cast<ASFCharacterBase>(Avatar)->SetCameraMode(AimingCameraModeTag);
	}

	// 3. Reticle(조준 장판) 소환
	if (ReticleClass && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnedReticle = GetWorld()->SpawnActor<AActor>(ReticleClass, GetAvatarActorFromActorInfo()->GetActorTransform(), Params);

		// [핵심 수정] 생성은 하되, 로컬 컨트롤러(시전자 본인)가 아니면 숨김 처리
		if (SpawnedReticle)
		{
			if (!IsLocallyControlled())
			{
				SpawnedReticle->SetActorHiddenInGame(true);
			}
		}
	}
	
	// 4. Tick 활성화 (마우스 추적용)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &USFGA_Hero_GroundAoE::TickReticle);
	}
	
	// 5. 추가 입력 대기
	InputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, false);
	if (InputReleaseTask)
	{
		InputReleaseTask->OnRelease.AddDynamic(this, &USFGA_Hero_GroundAoE::OnKeyReleased);
		InputReleaseTask->ReadyForActivation();
	}
}

void USFGA_Hero_GroundAoE::OnKeyReleased(float TimeWaited)
{
	// 키를 뗐으므로 Release Task 정리
	if (InputReleaseTask)
	{
		InputReleaseTask->EndTask();
		InputReleaseTask = nullptr;
	}

	// 이제 비로소 "다시 누르기(확정)"를 기다립니다.
	InputPressTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false); // false: 새로 눌러야 함
	if (InputPressTask)
	{
		InputPressTask->OnPress.AddDynamic(this, &USFGA_Hero_GroundAoE::OnConfirmInputPressed);
		InputPressTask->ReadyForActivation();
	}
}

void USFGA_Hero_GroundAoE::TickReticle()
{
	// 어빌리티가 종료되었으면 중단
	if (!IsActive())
	{
		return;
	}

	// 마우스 위치 추적 및 Reticle 업데이트
	FVector HitLocation;
	if (GetGroundLocationUnderCursor(HitLocation))
	{
		if (SpawnedReticle)
		{
			SpawnedReticle->SetActorLocation(HitLocation);
		}
		TargetLocation = HitLocation; // 현재 위치 저장
	}

	// 다음 틱 예약
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &USFGA_Hero_GroundAoE::TickReticle);
	}
}

void USFGA_Hero_GroundAoE::OnConfirmInputPressed(float TimeWaited)
{
	// 입력 확정!
	
	// 1. Press Task 정리
	if (InputPressTask)
	{
		InputPressTask->EndTask();
		InputPressTask = nullptr;
	}

	if (AimingMontageTask)
	{
		AimingMontageTask->EndTask();
		AimingMontageTask = nullptr;
	}
	
	// 2. 코스트 지불
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 3. Reticle 제거 (시각적으로만 숨김, EndAbility에서 파괴)
	if (SpawnedReticle)
	{
		SpawnedReticle->SetActorHiddenInGame(true);
	}
	
	// 4. 공격 몽타주 재생
	if (AttackMontage)
	{
		MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			AttackMontage
		);

		MontageTask->OnCompleted.AddDynamic(this, &USFGA_Hero_GroundAoE::OnAttackMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &USFGA_Hero_GroundAoE::OnAttackMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &USFGA_Hero_GroundAoE::OnAttackMontageCompleted);
		MontageTask->ReadyForActivation();
	}
	else
	{
		// 몽타주 없으면 즉시 소환
		FGameplayEventData DummyPayload;
		OnSpawnEventReceived(DummyPayload);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}

	// 5. 소환 이벤트 대기 (몽타주 노티파이)
	if (SpawnEventTag.IsValid())
	{
		WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			SpawnEventTag,
			nullptr,
			true,
			true
		);
		WaitEventTask->EventReceived.AddDynamic(this, &USFGA_Hero_GroundAoE::OnSpawnEventReceived);
		WaitEventTask->ReadyForActivation();
	}
}

void USFGA_Hero_GroundAoE::OnSpawnEventReceived(FGameplayEventData Payload)
{
	// 실제 액터 소환 로직
	if (HasAuthority(&CurrentActivationInfo))
	{
		if (AOEActorClass && GetWorld())
		{
			FActorSpawnParameters Params;
			Params.Owner = GetAvatarActorFromActorInfo();
			Params.Instigator = Cast<APawn>(Params.Owner);
			
			// Reticle의 위치(TargetLocation)에 소환
			FTransform SpawnTM(FRotator::ZeroRotator, TargetLocation);

			ASFGroundAOE* AOEActor = GetWorld()->SpawnActor<ASFGroundAOE>(AOEActorClass, SpawnTM, Params);
			if (AOEActor)
			{
				float FinalDamage = BaseDamage.GetValueAtLevel(GetAbilityLevel());
				
				AOEActor->InitAOE(
					GetAbilitySystemComponentFromActorInfo(),
					GetAvatarActorFromActorInfo(),
					FinalDamage,
					AOERadius,
					Duration,
					TickInterval
				);
			}
		}
	}
}

void USFGA_Hero_GroundAoE::OnAttackMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

bool USFGA_Hero_GroundAoE::GetGroundLocationUnderCursor(FVector& OutLocation)
{
	APlayerController* PC = Cast<APlayerController>(GetControllerFromActorInfo());
	
	// [수정] PlayerCameraManager는 함수가 아니라 변수입니다.
	// 그리고 null 체크를 추가하여 안전성을 높입니다.
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}

	FVector CameraLoc;
	FRotator CameraRot;
	
	// [수정] 괄호()를 없애고 멤버 변수(->PlayerCameraManager)로 접근합니다.
	PC->PlayerCameraManager->GetCameraViewPoint(CameraLoc, CameraRot);

	// 카메라 앞쪽으로 쏠 레이저의 길이
	const float TraceDistance = 2000.0f; 
	FVector TraceStart = CameraLoc;
	FVector TraceEnd = CameraLoc + (CameraRot.Vector() * TraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetAvatarActorFromActorInfo());
	if (SpawnedReticle) Params.AddIgnoredActor(SpawnedReticle);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		Params
	);

	if (bHit)
	{
		OutLocation = Hit.Location;
		return true;
	}

	return false;
}

void USFGA_Hero_GroundAoE::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 1. Reticle 파괴
	if (SpawnedReticle)
	{
		SpawnedReticle->Destroy();
		SpawnedReticle = nullptr;
	}

	// 2. Task 정리 (안전장치)
	if (InputReleaseTask) InputReleaseTask->EndTask();
	if (InputPressTask) InputPressTask->EndTask();

	// 3. 카메라 원복 (필요 시)
	// Cast<ASFCharacterBase>(Avatar)->ResetCameraMode();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}