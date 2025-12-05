#include "SFGA_Hero_AreaHeal_C.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"

#include "Character/SFCharacterBase.h"
#include "Engine/World.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/SkeletalMeshComponent.h"

USFGA_Hero_AreaHeal_C::USFGA_Hero_AreaHeal_C(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

//현재 장착 무기 메쉬 찾기 (ComponentTag "MainWeapon" 기준)
USkeletalMeshComponent* USFGA_Hero_AreaHeal_C::FindCurrentWeaponMesh(ASFCharacterBase* OwnerChar) const
{
	if (!OwnerChar || !OwnerChar->GetMesh())
	{
		return nullptr;
	}

	TArray<USceneComponent*> Children;
	OwnerChar->GetMesh()->GetChildrenComponents(true, Children);

	for (USceneComponent* Comp : Children)
	{
		if (USkeletalMeshComponent* Skm = Cast<USkeletalMeshComponent>(Comp))
		{
			// BP_OneHandSword의 SKM_OneHandSword_001 에 "MainWeapon" 태그 달았다고 가정
			if (Skm->ComponentTags.Contains(FName(TEXT("MainWeapon"))))
			{
				return Skm;
			}
		}
	}

	return nullptr;
}

//===============================Ability 실행===============================
void USFGA_Hero_AreaHeal_C::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ASFCharacterBase* OwnerChar = GetSFCharacterFromActorInfo();
	if (!OwnerChar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	//-------------------- Attack Trail 시작 ---------------------
	USkeletalMeshComponent* WeaponMesh = FindCurrentWeaponMesh(OwnerChar);

	if (SwordTrailFX && WeaponMesh)
	{
		TrailComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
			SwordTrailFX,
			WeaponMesh,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true, true);

		if (TrailComp)
		{
			TrailComp->SetVariableFloat(TEXT("User.Fade"), 1.0f);
		}

		TWeakObjectPtr<USkeletalMeshComponent> WeakWeaponMesh = WeaponMesh;

		OwnerChar->GetWorldTimerManager().SetTimer(
			TrailUpdateHandle,
			[this, WeakWeaponMesh]()
			{
				if (!TrailComp) return;
				if (!WeakWeaponMesh.IsValid()) return;

				USkeletalMeshComponent* Mesh = WeakWeaponMesh.Get();

				TrailComp->SetVariableVec3(TEXT("User.TrailStart"),
					Mesh->GetSocketLocation(TEXT("Trail_Start")));
				TrailComp->SetVariableVec3(TEXT("User.TrailEnd"),
					Mesh->GetSocketLocation(TEXT("Trail_End")));
			},
			0.01f, true);
	}
	//-----------------------------------------------------------

	//공격 몽타주 재생
	if (LightningMontage)
	{
		auto* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("LightningMontage"), LightningMontage);

		if (MontageTask)
		{
			MontageTask->OnCompleted.AddDynamic(this, &USFGA_Hero_AreaHeal_C::OnMontageEnded);
			MontageTask->OnInterrupted.AddDynamic(this, &USFGA_Hero_AreaHeal_C::OnMontageEnded);
			MontageTask->OnCancelled.AddDynamic(this, &USFGA_Hero_AreaHeal_C::OnMontageEnded);
			MontageTask->ReadyForActivation();
		}
	}

	//AnimNotify → GameplayEvent 대기
	auto* LightningEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		FGameplayTag::RequestGameplayTag(TEXT("Event.Skill.LightningStrike"))
	);

	if (LightningEvent)
	{
		LightningEvent->EventReceived.AddDynamic(this, &USFGA_Hero_AreaHeal_C::OnLightningImpact);
		LightningEvent->ReadyForActivation();
	}
}
//=========================================================================

//========================스킬 이펙트 + 타격 판정 처리=========================

void USFGA_Hero_AreaHeal_C::OnLightningImpact(FGameplayEventData Payload)
{
	ASFCharacterBase* OwnerChar = GetSFCharacterFromActorInfo();
	if (!OwnerChar) return;

	FVector StrikePos = OwnerChar->GetActorLocation() +
		OwnerChar->GetActorForwardVector() * StrikeDistance;

	FVector TraceStart = StrikePos + FVector(0,0,200);
	FVector TraceEnd   = StrikePos + FVector(0,0,-500);

	FHitResult GroundHit;
	FCollisionQueryParams Params(NAME_None, false, OwnerChar);

	bool bHitGround = GetWorld()->LineTraceSingleByChannel(
		GroundHit, TraceStart, TraceEnd, ECC_Visibility, Params);

	if (bHitGround)
	{
		StrikePos = GroundHit.ImpactPoint + FVector(0,0,5);
	}

	if (LightningEffect1)
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), LightningEffect1, StrikePos);
	if (LightningEffect2)
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), LightningEffect2, StrikePos);

	if (LightningSound1)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), LightningSound1, StrikePos);
	if (LightningSound2)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), LightningSound2, StrikePos);

	TArray<AActor*> HitActors;
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		StrikePos,
		StrikeRadius,
		{ UEngineTypes::ConvertToObjectType(ECC_Pawn) },
		ASFCharacterBase::StaticClass(),
		{ OwnerChar },
		HitActors
	);

	bool bOwnerIsPlayer = OwnerChar->Tags.Contains("Player");
	bool bOwnerIsEnemy  = OwnerChar->Tags.Contains("Enemy");

	for (AActor* HitActor : HitActors)
	{
		if (!HitActor || HitActor == OwnerChar) continue;

		ASFCharacterBase* TargetChar = Cast<ASFCharacterBase>(HitActor);
		if (!TargetChar) continue;

		bool bTargetIsPlayer = TargetChar->Tags.Contains("Player");
		bool bTargetIsEnemy  = TargetChar->Tags.Contains("Enemy");

		if ((bOwnerIsPlayer && bTargetIsPlayer) ||
			(bOwnerIsEnemy && bTargetIsEnemy))
			continue;

		FHitResult FakeHit;
		FakeHit.Location = StrikePos;
		FakeHit.ImpactPoint = TargetChar->GetActorLocation();
		FakeHit.ImpactNormal = (TargetChar->GetActorLocation() - StrikePos).GetSafeNormal();
		FakeHit.HitObjectHandle = FActorInstanceHandle(TargetChar);
		
		ProcessHitResult(FakeHit, BaseDamage, nullptr);

		//데미지 처리
		if (DamageGE)
		{
			auto* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetChar);
			if (TargetASC)
			{
				FGameplayEffectSpecHandle DamageSpec = MakeOutgoingGameplayEffectSpec(DamageGE, 1.f);
				if (DamageSpec.IsValid())
				{
					//BaseDamage 값을 ExecutionCalculation으로 전달
					DamageSpec.Data->SetSetByCallerMagnitude(
						SFGameplayTags::Data_Damage_BaseDamage, // 반드시 GameplayTag 맞춰야 작동
						BaseDamage
					);

					//히트 정보 전달 (크리 표시, UI 처리 등 가능)
					FGameplayEffectContextHandle Ctx = DamageSpec.Data->GetContext();
					Ctx.AddHitResult(FakeHit);
					DamageSpec.Data->SetContext(Ctx);

					TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());
				}
			}
		}
		//=========================================================================================

		if (DebuffGE)
		{
			auto* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetChar);
			if (TargetASC)
			{
				FGameplayEffectSpecHandle DebuffSpec = MakeOutgoingGameplayEffectSpec(DebuffGE, 1.f);
				if (DebuffSpec.IsValid())
				{
					FGameplayEffectContextHandle Ctx = DebuffSpec.Data->GetContext();
					Ctx.AddHitResult(FakeHit);
					DebuffSpec.Data->SetContext(Ctx);

					TargetASC->ApplyGameplayEffectSpecToSelf(*DebuffSpec.Data.Get());
				}
			}
		}
	}
}

//===============================Ability 종료===============================
void USFGA_Hero_AreaHeal_C::OnMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void USFGA_Hero_AreaHeal_C::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (TrailComp)
	{
		ASFCharacterBase* OwnerChar = nullptr;
		if (ActorInfo && ActorInfo->AvatarActor.IsValid())
		{
			OwnerChar = Cast<ASFCharacterBase>(ActorInfo->AvatarActor.Get());
		}

		if (OwnerChar)
		{
			OwnerChar->GetWorldTimerManager().ClearTimer(TrailUpdateHandle);
		}

		TWeakObjectPtr<UNiagaraComponent> WeakTrail = TrailComp;
		float Fade = 1.f;

		if (OwnerChar)
		{
			OwnerChar->GetWorldTimerManager().SetTimer(
				TrailFadeHandle,
				[this, WeakTrail, Fade]() mutable
				{
					if (!WeakTrail.IsValid()) return;

					Fade -= 0.04f;
					WeakTrail->SetVariableFloat(TEXT("User.Fade"), Fade);

					if (Fade <= 0.f)
					{
						WeakTrail->Deactivate();
						WeakTrail->DestroyComponent();
						TrailComp = nullptr;
					}
				},
				0.03f, true);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
//=========================================================================