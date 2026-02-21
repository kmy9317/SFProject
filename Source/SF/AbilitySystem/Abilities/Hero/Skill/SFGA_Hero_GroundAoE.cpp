#include "SFGA_Hero_GroundAoE.h"
#include "AbilitySystem/Abilities/Hero/Skill/SkillActor/SFGroundAOE.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h" 
#include "AbilitySystemComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "SFHeroSkillTags.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/GameplayEffect/Hero/EffectContext/SFTargetDataTypes.h"
#include "AbilitySystem/Tasks/SFAbilityTask_WaitCancelInput.h"
#include "GameFramework/PlayerController.h"
#include "Input/SFInputGameplayTags.h"
#include "System/SFPoolSubsystem.h"

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
	CancelInputTask = nullptr;
	
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
	
	// 로컬 전용: Reticle + TickReticle + 입력 Task
	if (IsLocallyControlled())
	{
		if (ReticleClass && GetWorld())
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnedReticle = GetWorld()->SpawnActor<AActor>(ReticleClass, GetAvatarActorFromActorInfo()->GetActorTransform(), Params);
		}
		
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(ReticleTimerHandle, this, &ThisClass::TickReticle, 1.f / 60.f, true);
		}
	}

	// 입력 대기
	InputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, false);
	if (InputReleaseTask)
	{
		InputReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnKeyReleased);
		InputReleaseTask->ReadyForActivation();
	}
	// 취소 입력 대기 Task
	CancelInputTask = USFAbilityTask_WaitCancelInput::WaitCancelInput(this);
	if (CancelInputTask)
	{
		CancelInputTask->OnCancelInput.AddDynamic(this, &ThisClass::OnCancelInputReceived);
		CancelInputTask->ReadyForActivation();
	}

	// 서버(원격 클라이언트 전용): TargetData 대기
	// 리슨 서버 호스트는 OnConfirmInputPressed에서 직접 진행하므로 불필요
	if (ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled())
	{
		USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
		if (ASC)
		{
			FAbilityTargetDataSetDelegate& TargetDataDelegate = ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey());
			ServerTargetDataDelegateHandle = TargetDataDelegate.AddUObject(this, &ThisClass::OnServerTargetDataReceived);
			ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationInfo.GetActivationPredictionKey());
		}
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
}

void USFGA_Hero_GroundAoE::OnConfirmInputPressed(float TimeWaited)
{
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

	// 확정 후 취소 입력 비활성화
	if (CancelInputTask)
	{
		CancelInputTask->EndTask();
		CancelInputTask = nullptr;
	}

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (!IsLocallyControlled())
	{
		return;
	}

	if (SpawnedReticle)
	{
		SpawnedReticle->SetActorHiddenInGame(true);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReticleTimerHandle);
	}

	if (HasAuthority(&CurrentActivationInfo))
	{
		// 리슨 서버 호스트: TargetData 불필요, 직접 진행
		PlayAttackAndWaitSpawn();
	}
	else
	{
		// 원격 클라이언트: 서버에 위치 전송 + 예측으로 즉시 몽타주 재생
		{
			FScopedPredictionWindow ScopedPrediction(GetAbilitySystemComponentFromActorInfo());
			FSFGameplayAbilityTargetData_Location* LocationData = new FSFGameplayAbilityTargetData_Location(TargetLocation);
			FGameplayAbilityTargetDataHandle DataHandle(LocationData);
			GetAbilitySystemComponentFromActorInfo()->ServerSetReplicatedTargetData(GetCurrentAbilitySpecHandle(),GetCurrentActivationInfo().GetActivationPredictionKey(),DataHandle,FGameplayTag(),GetAbilitySystemComponentFromActorInfo()->ScopedPredictionKey);
		}
		PlayAttackAndWaitSpawn();
	}
}

void USFGA_Hero_GroundAoE::OnServerTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle,FGameplayTag ActivationTag)
{
	USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	}

	const FSFGameplayAbilityTargetData_Location* LocationData = static_cast<const FSFGameplayAbilityTargetData_Location*>(DataHandle.Get(0));

	if (!LocationData)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	// 서버 유효성 검증
	if (!ValidateTargetLocation(LocationData->TargetLocation))
	{
		AActor* AvatarActor = GetAvatarActorFromActorInfo();
		if (AvatarActor)
		{
			// 부정 위치 → 캐릭터 전방 MaxCastRange 지점으로 보정
			TargetLocation = AvatarActor->GetActorLocation() + AvatarActor->GetActorForwardVector() * MaxCastRange;
		}
		else
		{
			CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
			return;
		}
	}
	else
	{
		TargetLocation = LocationData->TargetLocation;
	}

	PlayAttackAndWaitSpawn();
}

bool USFGA_Hero_GroundAoE::ValidateTargetLocation(const FVector& RequestedLocation) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return false;
	}

	// 1. 거리 제한 (네트워크 지연 허용 오차 포함)
	float AllowedRange = MaxCastRange * RangeToleranceMultiplier;
	float DistSq = FVector::DistSquared2D(AvatarActor->GetActorLocation(), RequestedLocation);
	if (DistSq > FMath::Square(AllowedRange))
	{
		return false;
	}

	// 2. 지면 존재 확인
	FHitResult Hit;
	FVector TraceStart = RequestedLocation + FVector(0.f, 0.f, 500.f);
	FVector TraceEnd = RequestedLocation - FVector(0.f, 0.f, 500.f);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(AvatarActor);

	if (!GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		return false;
	}

	return true;
}

void USFGA_Hero_GroundAoE::PlayAttackAndWaitSpawn()
{
	if (AttackMontage)
	{
		MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AttackMontage);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnAttackMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnAttackMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnAttackMontageCompleted);
		MontageTask->ReadyForActivation();
	}
	else
	{
		FGameplayEventData DummyPayload;
		OnSpawnEventReceived(DummyPayload);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}

	if (SpawnEventTag.IsValid())
	{
		WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SpawnEventTag, nullptr, true, true);
		WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnSpawnEventReceived);
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
			FTransform SpawnTM(FRotator::ZeroRotator, TargetLocation);
			USFPoolSubsystem* Pool = USFPoolSubsystem::Get(this);
			if (!Pool)
			{
				return;
			}
			ASFGroundAOE* AOEActor = Pool->AcquireActor<ASFGroundAOE>(AOEActorClass, SpawnTM);
			if (AOEActor)
			{
				// Owner/Instigator 재설정(풀 재사용 시 필요)
				AOEActor->SetOwner(GetAvatarActorFromActorInfo());
				AOEActor->SetInstigator(Cast<APawn>(GetAvatarActorFromActorInfo()));

				float FinalDamage = BaseDamage.GetValueAtLevel(GetAbilityLevel());
				AOEActor->InitAOE(GetAbilitySystemComponentFromActorInfo(), GetAvatarActorFromActorInfo(), FinalDamage, AOERadius, Duration, TickInterval);
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
	if (SpawnedReticle)
	{
		SpawnedReticle->Destroy();
		SpawnedReticle = nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReticleTimerHandle);
	}

	// 서버 TargetData 델리게이트 정리
	if (ActorInfo->IsNetAuthority() && ServerTargetDataDelegateHandle.IsValid())
	{
		if (USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo())
		{
			FAbilityTargetDataSetDelegate& Delegate = ASC->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());Delegate.Remove(ServerTargetDataDelegateHandle);
			ServerTargetDataDelegateHandle.Reset();
		}
	}

	if (InputReleaseTask) { InputReleaseTask->EndTask(); }
	if (InputPressTask)   { InputPressTask->EndTask(); }
	if (CancelInputTask)  { CancelInputTask->EndTask(); }

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}