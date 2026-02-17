#include "SFGA_Hero_ProjectileLaunch.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/SFCharacterBase.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "Actors/SFAttackProjectile.h"

#include "Components/MeshComponent.h"
#include "GameFramework/PlayerController.h"

USFGA_Hero_ProjectileLaunch::USFGA_Hero_ProjectileLaunch(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

float USFGA_Hero_ProjectileLaunch::GetScaledBaseDamage() const
{
	return BaseDamage.GetValueAtLevel(GetAbilityLevel());
}

void USFGA_Hero_ProjectileLaunch::ActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 필수 데이터 체크
	if (!ProjectileClass || !LaunchMontage || !ProjectileSpawnEventTag.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this,ProjectileSpawnEventTag,nullptr,true,true);
	if (WaitEventTask)
	{
		WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnProjectileSpawnEventReceived);
		WaitEventTask->ReadyForActivation();
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this,NAME_None,LaunchMontage,LaunchMontagePlayRate);

	if (MontageTask)
	{
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
		MontageTask->ReadyForActivation();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void USFGA_Hero_ProjectileLaunch::OnProjectileSpawnEventReceived(FGameplayEventData Payload)
{
	if (!IsActive())
	{
		return;
	}

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	FTransform SpawnTM;
	if (!GetProjectileSpawnTransform(SpawnTM))
	{
		if (ASFCharacterBase* Character = GetSFCharacterFromActorInfo())
		{
			SpawnTM = FTransform(Character->GetActorRotation(), Character->GetActorLocation() + FallbackSpawnOffset);
		}
	}

	const FVector LaunchDir = GetLaunchDirection();

	if (HasAuthority(&CurrentActivationInfo))
	{
		SpawnProjectile_Server(SpawnTM, LaunchDir);
	}
}

void USFGA_Hero_ProjectileLaunch::SpawnProjectile_Server(const FTransform& SpawnTM, const FVector& LaunchDir)
{
	if (!ProjectileClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	USFAbilitySystemComponent* SourceASC = GetSFAbilitySystemComponentFromActorInfo();
	if (!World || !Character || !SourceASC)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = Character;
	Params.Instigator = Cast<APawn>(Character);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASFAttackProjectile* Projectile = World->SpawnActorDeferred<ASFAttackProjectile>(
		ProjectileClass,
		SpawnTM,
		Params.Owner,
		Params.Instigator,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);
	
	const float Damage = GetScaledBaseDamage();
	Projectile->InitProjectile(SourceASC, Damage, Character);
	
	if (Projectile)
	{
		Projectile->FinishSpawning(SpawnTM);
	}
	
	if (!Projectile)
	{
		return;
	}
	
	Projectile->Launch(LaunchDir);
}

bool USFGA_Hero_ProjectileLaunch::GetProjectileSpawnTransform(FTransform& OutSpawnTM) const
{
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (!Character)
	{
		return false;
	}

	AActor* WeaponActor = GetMainHandWeaponActor();
	if (WeaponActor)
	{
		if (UMeshComponent* MeshComp = WeaponActor->FindComponentByClass<UMeshComponent>())
		{
			if (MeshComp->DoesSocketExist(SpawnSocketName))
			{
				OutSpawnTM = MeshComp->GetSocketTransform(SpawnSocketName, RTS_World);
				return true;
			}
		}

		OutSpawnTM = WeaponActor->GetActorTransform();
		return true;
	}

	// 무기 액터를 못 찾으면 캐릭터 기준
	OutSpawnTM = FTransform(Character->GetActorRotation(), Character->GetActorLocation() + FallbackSpawnOffset);
	return true;
}

FVector USFGA_Hero_ProjectileLaunch::GetLaunchDirection() const
{
	// 가능한 한 “바라보던 방향” (컨트롤 로테이션 우선)
	if (AController* Controller = GetControllerFromActorInfo())
	{
		const FRotator ControlRot = Controller->GetControlRotation();
		return ControlRot.Vector().GetSafeNormal();
	}

	if (ASFCharacterBase* Character = GetSFCharacterFromActorInfo())
	{
		return Character->GetActorForwardVector().GetSafeNormal();
	}

	return FVector::ForwardVector;
}

void USFGA_Hero_ProjectileLaunch::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Hero_ProjectileLaunch::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Hero_ProjectileLaunch::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Hero_ProjectileLaunch::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility)
{
	if (ScopeLockCount > 0)
	{
		WaitingToExecute.Add(FPostLockDelegate::CreateUObject(this, &USFGA_Hero_ProjectileLaunch::CancelAbility, Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility));
		return;
	}

	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void USFGA_Hero_ProjectileLaunch::EndAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,bool bReplicateEndAbility,bool bWasCancelled)
{
	if (WaitEventTask)
	{
		WaitEventTask->EventReceived.RemoveAll(this); // 델리게이트 해제 (필수)
		WaitEventTask->EndTask(); 
		WaitEventTask = nullptr;
	}

	if (MontageTask)
	{
		// 델리게이트를 먼저 끊어주어, EndTask()로 인한 몽타주 중지 이벤트가 
		// 다시 OnMontageInterrupted -> EndAbility 로 들어오는 루프를 차단합니다.
		MontageTask->OnBlendOut.RemoveAll(this);
		MontageTask->OnCompleted.RemoveAll(this);
		MontageTask->OnInterrupted.RemoveAll(this);
		MontageTask->OnCancelled.RemoveAll(this);

		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}