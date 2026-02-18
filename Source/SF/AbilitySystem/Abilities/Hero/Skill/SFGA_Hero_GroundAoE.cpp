#include "SFGA_Hero_GroundAoE.h"
#include "AbilitySystem/Abilities/Hero/Skill/SkillActor/SFGroundAOE.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h" // [추가] 필수 헤더
#include "GameFramework/PlayerController.h"
#include "AbilitySystemComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "InputCoreTypes.h"
#include "SFHeroSkillTags.h"
#include "AbilitySystem/Tasks/SFAbilityTask_WaitCancelInput.h"
#include "GameFramework/PlayerController.h"
#include "Input/SFInputGameplayTags.h"

USFGA_Hero_GroundAoE::USFGA_Hero_GroundAoE(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	CancelOnInputTags.AddTag(SFGameplayTags::InputTag_RightAttack);

	bServerRespectsRemoteAbilityCancellation = true;
}

void USFGA_Hero_GroundAoE::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	MontageTask = nullptr;
	AimingMontageTask = nullptr;
	SpawnedReticle = nullptr;
	InputReleaseTask = nullptr;
	InputPressTask = nullptr;
	WaitEventTask = nullptr;
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitCheck(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		TargetLocation = Avatar->GetActorLocation();
	}

	if (AimingLoopMontage)
	{
		AimingMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this,NAME_None,AimingLoopMontage,1.0f,NAME_None,false );
		AimingMontageTask->ReadyForActivation();
	}

	// Reticle(조준 장판) 소환
	if (ReticleClass && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnedReticle = GetWorld()->SpawnActor<AActor>(ReticleClass, GetAvatarActorFromActorInfo()->GetActorTransform(), Params);

		// 생성은 하되 로컬 컨트롤러(시전자 본인)가 아니면 숨김 처리
		if (SpawnedReticle)
		{
			if (!IsLocallyControlled())
			{
				SpawnedReticle->SetActorHiddenInGame(true);
			}
		}
	}
	
	// Tick 활성화 (마우스 추적용)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &USFGA_Hero_GroundAoE::TickReticle);
	}

	// 취소 입력 대기 Task
	CancelInputTask = USFAbilityTask_WaitCancelInput::WaitCancelInput(this);
	if (CancelInputTask)
	{
		CancelInputTask->OnCancelInput.AddDynamic(this, &ThisClass::OnCancelInputReceived);
		CancelInputTask->ReadyForActivation();
	}
	
	// 입력 대기
	InputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, false);
	if (InputReleaseTask)
	{
		InputReleaseTask->OnRelease.AddDynamic(this, &USFGA_Hero_GroundAoE::OnKeyReleased);
		InputReleaseTask->ReadyForActivation();
	}
}

void USFGA_Hero_GroundAoE::OnCancelInputReceived()
{
	// 디바운싱 (클라이언트 + 서버 양쪽에서 실행됨)
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(SFGameplayTags::Ability_Skill_Activated);

		TWeakObjectPtr<UAbilitySystemComponent> WeakASC = ASC;
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [WeakASC]()
		{
			if (WeakASC.IsValid())
			{
				WeakASC->RemoveLooseGameplayTag(SFGameplayTags::Ability_Skill_Activated);
			}
		}, 0.3f, false);
	}

	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false);
}

void USFGA_Hero_GroundAoE::OnKeyReleased(float TimeWaited)
{
	// 키를 뗐으므로 Release Task 정리
	if (InputReleaseTask)
	{
		InputReleaseTask->EndTask();
		InputReleaseTask = nullptr;
	}

	// 이제 비로소 "다시 누르기(확정)"를 기다림
	InputPressTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false); // false: 새로 눌러야 함
	if (InputPressTask)
	{
		InputPressTask->OnPress.AddDynamic(this, &USFGA_Hero_GroundAoE::OnConfirmInputPressed);
		InputPressTask->ReadyForActivation();
	}
}

void USFGA_Hero_GroundAoE::TickReticle()
{
	if (!IsActive() || MontageTask)
	{
		return;
	}

	FVector HitLocation;
	if (GetGroundLocationUnderCursor(HitLocation))
	{
		if (SpawnedReticle)
		{
			SpawnedReticle->SetActorLocation(HitLocation);
		}
		TargetLocation = HitLocation;
	}

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
		MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this,NAME_None,AttackMontage);

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
		WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SpawnEventTag,nullptr,true,true);
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
				AOEActor->InitAOE(GetAbilitySystemComponentFromActorInfo(), GetAvatarActorFromActorInfo(),FinalDamage,AOERadius,Duration,TickInterval);
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
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}

	FVector CameraLoc;
	FRotator CameraRot;
	PC->PlayerCameraManager->GetCameraViewPoint(CameraLoc, CameraRot);

	// 카메라 앞쪽으로 쏠 레이저의 길이
	const float TraceDistance = 2000.0f; 
	FVector TraceStart = CameraLoc;
	FVector TraceEnd = CameraLoc + (CameraRot.Vector() * TraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetAvatarActorFromActorInfo());
	if (SpawnedReticle)
	{
		Params.AddIgnoredActor(SpawnedReticle);
	}
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit,TraceStart,TraceEnd,ECC_Visibility,Params);
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
	if (InputReleaseTask)
	{
		InputReleaseTask->EndTask();
	}
	if (InputPressTask)
	{
		InputPressTask->EndTask();
	}
	if (CancelInputTask)
	{
		CancelInputTask->EndTask();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}