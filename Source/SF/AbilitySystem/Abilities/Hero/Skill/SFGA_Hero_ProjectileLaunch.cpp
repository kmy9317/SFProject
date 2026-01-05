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

void USFGA_Hero_ProjectileLaunch::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 필수 데이터 체크
	if (!ProjectileClass || !LaunchMontage || !ProjectileSpawnEventTag.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 노티파이(게임플레이 이벤트) 대기
	WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		ProjectileSpawnEventTag,
		nullptr,
		/*OnlyTriggerOnce*/ true,
		/*OnlyMatchExact*/ true
	);

	if (WaitEventTask)
	{
		WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnProjectileSpawnEventReceived);
		WaitEventTask->ReadyForActivation();
	}

	// 2. 몽타주 재생
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		LaunchMontage,
		LaunchMontagePlayRate
	);

	if (MontageTask)
	{
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
	// 만약 시전 도중 마나가 부족해졌거나 조건이 안 맞으면 발사 실패 처리합니다.
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		// 코스트 지불 실패 시 (혹은 쿨타임 문제 등) 어빌리티 취소
		K2_CancelAbility();
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

	// 주의: 여기서 EndAbility를 호출하지 않고 몽타주 종료를 기다립니다.
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

	ASFAttackProjectile* Projectile = World->SpawnActor<ASFAttackProjectile>(
		ProjectileClass,
		SpawnTM.GetLocation(),
		LaunchDir.Rotation(),
		Params
	);

	if (!Projectile)
	{
		return;
	}

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
		if (UMeshComponent* MeshComp = WeaponActor->FindComponentByClass<UMeshComponent>())
		{
			if (MeshComp->DoesSocketExist(SpawnSocketName))
			{
				OutSpawnTM = MeshComp->GetSocketTransform(SpawnSocketName, RTS_World);
				return true;
			}
		}

		// 무기엔 소켓이 없지만 위치는 무기 기준으로라도
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

void USFGA_Hero_ProjectileLaunch::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled
)
{
	if (WaitEventTask)
	{
		WaitEventTask->EventReceived.RemoveAll(this);
		WaitEventTask = nullptr;
	}

	if (MontageTask)
	{
		MontageTask->OnCompleted.RemoveAll(this);
		MontageTask->OnInterrupted.RemoveAll(this);
		MontageTask->OnCancelled.RemoveAll(this);
		MontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}