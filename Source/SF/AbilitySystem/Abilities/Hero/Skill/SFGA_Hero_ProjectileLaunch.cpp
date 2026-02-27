#include "SFGA_Hero_ProjectileLaunch.h"

#include "SFLogChannels.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/SFCharacterBase.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "Actors/SFAttackProjectile.h"

#include "Components/MeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "System/SFPoolSubsystem.h"

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

	if (!ValidateLaunchRequirements())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartLaunchSequence();
}

bool USFGA_Hero_ProjectileLaunch::ValidateLaunchRequirements() const
{
	if (!ProjectileClass)
	{
		return false;
	}

	if (!LaunchMontage)
	{
		return false;
	}

	if (!ProjectileSpawnEventTag.IsValid())
	{
		return false;
	}

	return true;
}

void USFGA_Hero_ProjectileLaunch::StartLaunchSequence()
{
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
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
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
	
	ASFAttackProjectile* Projectile = USFPoolSubsystem::Get(this)->AcquireActor<ASFAttackProjectile>(ProjectileClass, SpawnTM);
	if (!Projectile)
	{
		return;
	}
	Projectile->SetOwner(Character);
	Projectile->SetInstigator(Cast<APawn>(Character));

	const float Damage = GetScaledBaseDamage();
	Projectile->InitProjectile(SourceASC, Damage, Character);
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
		TArray<UMeshComponent*> MeshComponents;
		WeaponActor->GetComponents<UMeshComponent>(MeshComponents);
		for (UMeshComponent* MeshComp : MeshComponents)
		{
			if (MeshComp && MeshComp->DoesSocketExist(SpawnSocketName))
			{
				OutSpawnTM = MeshComp->GetSocketTransform(SpawnSocketName, RTS_World);

				UE_LOG(LogSF, Warning, TEXT("[Projectile] Socket:%s Loc:%s CharLoc:%s Delta:%f"),
					*SpawnSocketName.ToString(),
					*OutSpawnTM.GetLocation().ToString(),
					*Character->GetActorLocation().ToString(),
					FVector::Dist(OutSpawnTM.GetLocation(), Character->GetActorLocation()));
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

void USFGA_Hero_ProjectileLaunch::CleanupLaunchTasks()
{
	if (WaitEventTask)
	{
		WaitEventTask->EventReceived.RemoveAll(this);
		WaitEventTask->EndTask();
		WaitEventTask = nullptr;
	}

	if (MontageTask)
	{
		MontageTask->OnBlendOut.RemoveAll(this);
		MontageTask->OnCompleted.RemoveAll(this);
		MontageTask->OnInterrupted.RemoveAll(this);
		MontageTask->OnCancelled.RemoveAll(this);
		MontageTask->EndTask();
		MontageTask = nullptr;
	}
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

TArray<FSFPoolPrewarmEntry> USFGA_Hero_ProjectileLaunch::GetPoolPrewarmEntries() const
{
	TArray<FSFPoolPrewarmEntry> Entries;
	if (ProjectileClass && PoolCountPerPlayer)
	{
		Entries.Add({ ProjectileClass, PoolCountPerPlayer });
	}
	return Entries;
}

void USFGA_Hero_ProjectileLaunch::EndAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,bool bReplicateEndAbility,bool bWasCancelled)
{
	CleanupLaunchTasks();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
