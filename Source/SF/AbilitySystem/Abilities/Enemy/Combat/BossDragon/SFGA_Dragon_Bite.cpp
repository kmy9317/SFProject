// Fill out your copyright notice in the Description page of Project Settings.

#include "SFGA_Dragon_Bite.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterBase.h"

USFGA_Dragon_Bite::USFGA_Dragon_Bite()
{
	
	AbilityID = FName("Dragon_Bite");
	
	AttackType = EAttackType::Melee;
}

void USFGA_Dragon_Bite::ActivateAbility( const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (BiteMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			BiteMontage,
			1.0f,
			NAME_None,
			true
		);

		if (MontageTask)
		{
			MontageTask->OnCompleted.AddDynamic(this, &USFGA_Dragon_Bite::OnBiteMontageCompleted);
			MontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageInterrupted);
			MontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageCancelled);
			MontageTask->ReadyForActivation();
			CurrentBiteCount = 0;
		}
	}

	// Event는 서버에서 
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



void USFGA_Dragon_Bite::OnBiteMontageCompleted()
{
	
	if (GrabbedTarget.IsValid() && GrabMontage)
	{
		PlayGrabMontage();
	}
	else
	{

		CurrentBiteCount++;


		if (CurrentBiteCount < BiteCount)
		{
			UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this,
				NAME_None,
				BiteMontage,
				1.0f,
				NAME_None,
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
		else
		{

			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		}
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
        GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(
            *GrabSpec.Data.Get(),
            TargetASC
        );
    }

	CurrentBiteCount = 0;
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
    
    
    FAttachmentTransformRules AttachRules(
        EAttachmentRule::SnapToTarget,  
        EAttachmentRule::SnapToTarget,  
        EAttachmentRule::KeepWorld,     
        true                             
    );
    
    Target->AttachToComponent(
        DragonMesh,
        AttachRules,
        JawSocketName
    );
    
    // 플레이어 입력 차단
    if (APlayerController* PC = Cast<APlayerController>(Target->GetInstigatorController()))
    {
        PC->SetIgnoreMoveInput(true);
        PC->SetIgnoreLookInput(true);
    }
	
}

void USFGA_Dragon_Bite::DetachTarget(AActor* Target)
{
    if (!Target)
        return;
	
    FDetachmentTransformRules DetachRules(
        EDetachmentRule::KeepWorld,  
        EDetachmentRule::KeepWorld,  
        EDetachmentRule::KeepWorld,  
        true                         
    );
    
    Target->DetachFromActor(DetachRules);
    
    // 플레이어 입력 복구
    if (APlayerController* PC = Cast<APlayerController>(Target->GetInstigatorController()))
    {
        PC->SetIgnoreMoveInput(false);
        PC->SetIgnoreLookInput(false);
    }

}

void USFGA_Dragon_Bite::PlayGrabMontage()
{
	if (!GrabMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}
	
	UAbilityTask_PlayMontageAndWait* GrabMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		GrabMontage,
		1.0f,
		NAME_None,
		true
	);

	if (GrabMontageTask)
	{
		GrabMontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageInterrupted);
		GrabMontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageCancelled);
		GrabMontageTask->ReadyForActivation();

		GetWorld()->GetTimerManager().SetTimer(
			GrabDurationTimerHandle,
			this,
			&USFGA_Dragon_Bite::OnGrabMontageCompleted,
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
			CurrentBiteCount++;

			if (CurrentBiteCount >= RescueCount)
			{
				ApplyStaggerToSelf();

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
