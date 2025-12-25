// Fill out your copyright notice in the Description page of Project Settings.

#include "SFGA_Dragon_Bite.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterBase.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"
#include "Components/CapsuleComponent.h"

USFGA_Dragon_Bite::USFGA_Dragon_Bite()
{
	AbilityID = FName("Dragon_Bite");
	AttackType = EAttackType::Melee;

	// Ability Tags
	AbilityTags.AddTag(SFGameplayTags::Ability_Dragon_Bite);

	// Cooldown Tag
	CoolDownTag = SFGameplayTags::Ability_Cooldown_Dragon_Bite;
}

void USFGA_Dragon_Bite::ActivateAbility( const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentBiteCount = 0;

	StartBiteAttack();

	if (!ActorInfo->IsNetAuthority())
	{
		return;
	}

	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		SFGameplayTags::GameplayEvent_Tracing,
		nullptr,
		true,
		true
	);

	if (WaitEventTask)
	{
		WaitEventTask->EventReceived.AddDynamic(this, &USFGA_Dragon_Bite::OnBiteHit);
		WaitEventTask->ReadyForActivation();
	}
}

void USFGA_Dragon_Bite::StartBiteAttack()
{
	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (!Dragon || !Dragon->GetMesh() || !BiteMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UAnimInstance* AnimInst = Dragon->GetMesh()->GetAnimInstance();
	if (!AnimInst)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// Play BiteMontage
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		BiteMontage,
		1.0f,
		FName("BiteStart"),
		true
	);

	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &USFGA_Dragon_Bite::OnBiteMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageCancelled);
		MontageTask->ReadyForActivation();
	}
}



void USFGA_Dragon_Bite::OnBiteMontageCompleted()
{

	if (GrabbedTarget.IsValid())
	{
		PlayGrabMontage();
		return;
	}

	CurrentBiteCount++;

	if (CurrentBiteCount < BiteCount)
	{
		PlayBiteLoop();
	}
	else
	{

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void USFGA_Dragon_Bite::PlayBiteLoop()
{
	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (!Dragon || !Dragon->GetMesh() || !BiteMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UAnimInstance* AnimInst = Dragon->GetMesh()->GetAnimInstance();
	if (!AnimInst)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// Play BiteLoop section
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		BiteMontage,
		1.0f,
		FName("BiteLoop"),
		false
	);

	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &USFGA_Dragon_Bite::OnBiteMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageCancelled);
		MontageTask->ReadyForActivation();
	}
}

void USFGA_Dragon_Bite::OnGrabMontageCompleted()
{
	if (GrabbedTarget.IsValid())
	{
		ApplyExecutionDamage(GrabbedTarget.Get());
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Dragon_Bite::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Dragon_Bite::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}


void USFGA_Dragon_Bite::OnBiteHit(FGameplayEventData Payload)
{
    const FHitResult* HitResult = Payload.ContextHandle.GetHitResult();
    if (!HitResult) return;
	
    AActor* HitActor = HitResult->GetActor();
    if (!HitActor) return;

    if (GetAttitudeTowards(HitActor) != ETeamAttitude::Hostile)
        return;

    if (GrabbedTarget.IsValid())
    {
        return;
    }

    FGameplayEffectContextHandle EffectContext =
        MakeEffectContext(CurrentSpecHandle, CurrentActorInfo);
    EffectContext.AddHitResult(*HitResult);

    ApplyDamageToTarget(HitActor, EffectContext);

    ApplyGrabEffect(HitActor);

    AttachTargetToJaw(HitActor);

    GrabbedTarget = HitActor;

    // Pressure 적용 (ISFDragonPressureInterface)
    ApplyPressureToTarget(HitActor);
}



void USFGA_Dragon_Bite::ApplyGrabEffect(AActor* Target)
{
    if (!Target || !GrabGameplayEffectClass)
        return;

    UAbilitySystemComponent* TargetASC =
        UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

    if (!TargetASC)
        return;

    FGameplayEffectSpecHandle GrabSpec =
        MakeOutgoingGameplayEffectSpec(GrabGameplayEffectClass, 1);

    if (GrabSpec.IsValid())
    {
        ActiveGrabEffectHandle = GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(
            *GrabSpec.Data.Get(),
            TargetASC
        );
    }

	CurrentHitCount = 0;
	LastDamageTime = -999.0f;

	OnDamageRecivedHandle = GetAbilitySystemComponentFromActorInfo()
		->OnGameplayEffectAppliedDelegateToSelf.AddUObject(
			this,
			&ThisClass::OnDamageRecieved
		);
}

void USFGA_Dragon_Bite::AttachTargetToJaw(AActor* Target)
{
    if (!Target)
        return;

    ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
    if (!Dragon)
        return;

    USkeletalMeshComponent* DragonMesh = Dragon->GetMesh();
    if (!DragonMesh)
        return;

	// --> TODO GrabbedAbiltiy에서 하도록 수정
	if (ACharacter* Char = Cast<ACharacter>(Target))
	{
		Char->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	// 플레이어 입력 차단 -> 이건 아마 Grab태그를 Player한테 주는 방향으로 수정할 듯
	if (APlayerController* PC = Cast<APlayerController>(Target->GetInstigatorController()))
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}

	// 
	

	Target->AttachToComponent(
	 DragonMesh,
	 FAttachmentTransformRules::SnapToTargetNotIncludingScale,
	 JawSocketName
 );



}

void USFGA_Dragon_Bite::DetachTarget(AActor* Target)
{
	if (!Target)
		return;

	ACharacter* Char = Cast<ACharacter>(Target);

	
	if (ActiveGrabEffectHandle.IsValid())
	{
		if (UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
		{
			TargetASC->RemoveActiveGameplayEffect(ActiveGrabEffectHandle);
		}
		ActiveGrabEffectHandle.Invalidate();
	}
	
	Target->DetachFromActor(
		FDetachmentTransformRules::KeepWorldTransform
	);

	// 이것도 Ability End 쪽에서 
	if (Char)
	{
		UCapsuleComponent* Capsule = Char->GetCapsuleComponent();
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		
		Char->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	}
	
	if (APlayerController* PC = Cast<APlayerController>(Target->GetInstigatorController()))
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}
	// 
}


void USFGA_Dragon_Bite::PlayGrabMontage()
{
	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (!Dragon || !Dragon->GetMesh() || !BiteMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UAnimInstance* AnimInst = Dragon->GetMesh()->GetAnimInstance();
	if (!AnimInst)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// Play GrabLoop section from BiteMontage (loop until duration expires or rescue succeeds)
	UAbilityTask_PlayMontageAndWait* GrabMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		BiteMontage,  // Use BiteMontage
		1.0f,
		FName("GrabLoop"),  // Start from GrabLoop section
		true
	);

	if (GrabMontageTask)
	{
		GrabMontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageInterrupted);
		GrabMontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageCancelled);
		GrabMontageTask->ReadyForActivation();

		// Set timer for GrabDuration (5 seconds)
		GetWorld()->GetTimerManager().SetTimer(
			GrabDurationTimerHandle,
			this,
			&USFGA_Dragon_Bite::OnGrabMontageCompleted,  // Directly call OnGrabMontageCompleted
			GrabDuration,
			false
		);
	}
}

void USFGA_Dragon_Bite::OnDamageRecieved(UAbilitySystemComponent* Source, const FGameplayEffectSpec& SpecApplied,
	FActiveGameplayEffectHandle ActiveHandle)
{
	if (!GrabbedTarget.IsValid())
	{
		return;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastDamageTime < DamageCountCoolDown)
	{
		return;
	}

	for (const FGameplayModifierInfo& Modifier : SpecApplied.Def->Modifiers)
	{
		if (Modifier.Attribute.AttributeName == "Damage")
		{
			LastDamageTime = CurrentTime;
			CurrentHitCount++;

			if (CurrentHitCount >= RescueCount)
			{
				
				if (GrabDurationTimerHandle.IsValid())
				{
					GetWorld()->GetTimerManager().ClearTimer(GrabDurationTimerHandle);
				}
				
				ApplyStaggerToSelf();
				
				ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
				if (Dragon && Dragon->GetMesh())
				{
					UAnimInstance* AnimInst = Dragon->GetMesh()->GetAnimInstance();
					if (AnimInst && BiteMontage)
					{
						AnimInst->Montage_Stop(0.2f, BiteMontage);
					}
				}

				EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			}
			break;
		}
	}
}


void USFGA_Dragon_Bite::ApplyExecutionDamage(AActor* Target)
{
	if (!Target || !DamageGameplayEffectClass)
		return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
		return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
		return;

	FGameplayEffectSpecHandle SpecHandle =
		MakeOutgoingGameplayEffectSpec(DamageGameplayEffectClass, GetAbilityLevel());

	if (!SpecHandle.IsValid())
		return;

	SpecHandle.Data->SetSetByCallerMagnitude(
		SFGameplayTags::Data_Damage_BaseDamage,
		ExecutionDamage
	);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

void USFGA_Dragon_Bite::ApplyStaggerToSelf()
{
	if (!StaggerGameplayEffectClass)
		return;

	UAbilitySystemComponent* DragonASC = GetAbilitySystemComponentFromActorInfo();
	if (!DragonASC)
		return;

	FGameplayEffectSpecHandle StaggerSpec = MakeOutgoingGameplayEffectSpec(StaggerGameplayEffectClass, 1);

	if (StaggerSpec.IsValid())
	{
		StaggerSpec.Data->SetSetByCallerMagnitude(
			SFGameplayTags::Data_Stagger_BaseStagger,
			StaggerDamageOnRescue
		);

		DragonASC->ApplyGameplayEffectSpecToSelf(*StaggerSpec.Data.Get());
	}
}

void USFGA_Dragon_Bite::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{

	if (GrabbedTarget.IsValid())
	{
		DetachTarget(GrabbedTarget.Get());
		GrabbedTarget.Reset();
	}

	if (GrabDurationTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(GrabDurationTimerHandle);
	}

	if (OnDamageRecivedHandle.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->OnGameplayEffectAppliedDelegateToSelf.Remove(OnDamageRecivedHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


float USFGA_Dragon_Bite::CalcScoreModifier(const FEnemyAbilitySelectContext& Context) const
{
	float Modifier = 0.f;

	const FBossEnemyAbilitySelectContext* BossContext =
		static_cast<const FBossEnemyAbilitySelectContext*>(&Context);

	// Melee Zone이면 보너스 점수
	if (BossContext && BossContext->Zone == EBossAttackZone::Melee)
	{
		Modifier += 1000.f;
	}

	if (!Context.Target)
		return Modifier;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Context.Target);

	if (!TargetASC)
		return Modifier;

	// 플레이어가 Pressure_Back 상태면 점수 증가
	if (TargetASC->HasMatchingGameplayTag(SFGameplayTags::Dragon_Pressure_Back))
	{
		Modifier += 500.f;
	}

	// 이미 Forward Pressure 중이면 감소
	if (TargetASC->HasMatchingGameplayTag(SFGameplayTags::Dragon_Pressure_Forward))
	{
		Modifier -= 300.f;
	}

	return Modifier;
}
