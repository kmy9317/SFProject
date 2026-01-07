#include "SFGA_AreaOverTimeSkill.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// [중요] GroundAOE 로직 이식을 위한 필수 헤더
#include "System/SFAssetManager.h"
#include "System/Data/SFGameData.h"
#include "Character/SFCharacterBase.h"

USFGA_AreaOverTimeSkill::USFGA_AreaOverTimeSkill()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 기본 태그 설정
	DamageSetByCallerTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage.BaseDamage"), false);
	BaseDamage.Value = 10.0f;
}

void USFGA_AreaOverTimeSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlaySkillAnimation();
}

void USFGA_AreaOverTimeSkill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LoopTimerHandle);
		World->GetTimerManager().ClearTimer(DurationTimerHandle);
	}

	if (LoopingGameplayCueTag.IsValid())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			ASC->RemoveGameplayCue(LoopingGameplayCueTag);
		}
	}

	ClearAllFrozenTargets();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGA_AreaOverTimeSkill::PlaySkillAnimation()
{
	if (!SkillMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, SkillMontage, 1.0f, NAME_None, false, 1.0f
	);

	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCompleted);
		MontageTask->ReadyForActivation();
	}

	if (TriggerEventTag.IsValid())
	{
		WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TriggerEventTag);
		if (WaitEventTask)
		{
			WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnGameplayEventReceived);
			WaitEventTask->ReadyForActivation();
		}
	}
	else
	{
		StartAttackLoop();
	}
}

void USFGA_AreaOverTimeSkill::OnMontageCompleted()
{
	bool bIsLoopActive = GetWorld()->GetTimerManager().IsTimerActive(LoopTimerHandle);
	if (!bIsLoopActive)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void USFGA_AreaOverTimeSkill::OnGameplayEventReceived(FGameplayEventData Payload)
{
	StartAttackLoop();
}

void USFGA_AreaOverTimeSkill::StartAttackLoop()
{
	if (LoopingGameplayCueTag.IsValid())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = GetAvatarActorFromActorInfo()->GetActorLocation();
			ASC->AddGameplayCue(LoopingGameplayCueTag, CueParams);
		}
	}

	PerformAreaTick();

	GetWorld()->GetTimerManager().SetTimer(
		LoopTimerHandle,
		this,
		&ThisClass::PerformAreaTick,
		DamageInterval,
		true
	);

	GetWorld()->GetTimerManager().SetTimer(
		DurationTimerHandle,
		this,
		&ThisClass::OnDurationExpired,
		AttackDuration,
		false
	);
}

void USFGA_AreaOverTimeSkill::OnDurationExpired()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_AreaOverTimeSkill::PerformAreaTick()
{
	// 권한 체크: 서버에서만 실행
	if (!HasAuthority(&CurrentActivationInfo)) return;

	float DamageAmount = BaseDamage.GetValueAtLevel(GetAbilityLevel());
	
	// GroundAOE 로직 함수 호출
	ApplyDamageToTargets(DamageAmount, Radius);
}

// [핵심 구현] ASFGroundAOE::ApplyDamageToTargets 로직을 Ability에 맞춰 이식
void USFGA_AreaOverTimeSkill::ApplyDamageToTargets(float DamageAmount, float EffectRadius)
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC) return;

	AActor* AvatarActor = GetAvatarActorFromActorInfo(); // == SourceActor
	if (!AvatarActor) return;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor); // 시전자 무시

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	// 1. 충돌 감지 (ObjectType 사용)
	bool bHit = GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		AvatarActor->GetActorLocation(),
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(EffectRadius),
		QueryParams
	);

	if (!bHit) return;

	// 2. GE 클래스 결정 (폴백 로직 포함)
	TSubclassOf<UGameplayEffect> DamageGE = DamageGameplayEffectClass;
	if (!DamageGE)
	{
		// 블루프린트에서 설정이 안되어있으면 데이터 에셋에서 가져옴
		DamageGE = USFAssetManager::GetSubclassByPath(USFGameData::Get().DamageGameplayEffect_SetByCaller);
	}

	if (!DamageGE) return;

	TSet<AActor*> ProcessedActors;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!TargetActor || ProcessedActors.Contains(TargetActor)) continue;

		ProcessedActors.Add(TargetActor);

		// 3. 아군 피격 방지 로직 (ASFCharacterBase 캐스팅)
		if (ASFCharacterBase* SourceChar = Cast<ASFCharacterBase>(AvatarActor))
		{
			if (ASFCharacterBase* TargetChar = Cast<ASFCharacterBase>(TargetActor))
			{
				if (SourceChar->GetTeamAttitudeTowards(*TargetChar) == ETeamAttitude::Friendly)
				{
					continue; 
				}
			}
		}

		// 4. GE 적용
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (TargetASC)
		{
			FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
			ContextHandle.AddInstigator(AvatarActor, AvatarActor); // EffectCauser를 AvatarActor로 설정
          
			FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageGE, 1.0f, ContextHandle);
			if (SpecHandle.IsValid())
			{
				if (DamageSetByCallerTag.IsValid())
				{
					SpecHandle.Data->SetSetByCallerMagnitude(DamageSetByCallerTag, DamageAmount);
				}
				SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			}

			// 디버프 적용
			if (DebuffGameplayEffectClass)
			{
				FGameplayEffectSpecHandle DebuffSpec = SourceASC->MakeOutgoingSpec(DebuffGameplayEffectClass, 1.0f, ContextHandle);
				if (DebuffSpec.IsValid())
				{
					SourceASC->ApplyGameplayEffectSpecToTarget(*DebuffSpec.Data.Get(), TargetASC);
				}
			}

			// 빙결 처리 (추가 요구사항)
			if (bEnableFreeze)
			{
				FreezeTarget(TargetActor);
			}
		}
	}
}

void USFGA_AreaOverTimeSkill::FreezeTarget(AActor* TargetActor)
{
	if (!IsValid(TargetActor)) return;

	TWeakObjectPtr<AActor> WeakTarget(TargetActor);
	FFrozenTargetData* ExistingData = FrozenTargetMap.Find(WeakTarget);

	float FreezeDuration = DamageInterval + 0.2f;

	if (ExistingData)
	{
		GetWorld()->GetTimerManager().ClearTimer(ExistingData->UnfreezeTimerHandle);
		
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(this, &ThisClass::UnfreezeTarget, WeakTarget);
		GetWorld()->GetTimerManager().SetTimer(ExistingData->UnfreezeTimerHandle, TimerDelegate, FreezeDuration, false);
	}
	else
	{
		FFrozenTargetData NewData;
		NewData.OriginalTimeDilation = TargetActor->CustomTimeDilation;
		
		TargetActor->CustomTimeDilation = 0.0f;
		
		if (ACharacter* Char = Cast<ACharacter>(TargetActor))
		{
			if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
			{
				CMC->StopMovementImmediately();
			}
		}

		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(this, &ThisClass::UnfreezeTarget, WeakTarget);
		GetWorld()->GetTimerManager().SetTimer(NewData.UnfreezeTimerHandle, TimerDelegate, FreezeDuration, false);

		FrozenTargetMap.Add(WeakTarget, NewData);
	}
}

void USFGA_AreaOverTimeSkill::UnfreezeTarget(TWeakObjectPtr<AActor> TargetWeakPtr)
{
	if (FrozenTargetMap.Contains(TargetWeakPtr))
	{
		FFrozenTargetData Data = FrozenTargetMap[TargetWeakPtr];
		FrozenTargetMap.Remove(TargetWeakPtr); 

		if (AActor* TargetActor = TargetWeakPtr.Get())
		{
			if (IsValid(TargetActor))
			{
				TargetActor->CustomTimeDilation = Data.OriginalTimeDilation;
			}
		}
	}
}

void USFGA_AreaOverTimeSkill::ClearAllFrozenTargets()
{
	TArray<TWeakObjectPtr<AActor>> TargetsToUnfreeze;
	FrozenTargetMap.GetKeys(TargetsToUnfreeze);

	for (const TWeakObjectPtr<AActor>& TargetRef : TargetsToUnfreeze)
	{
		if (FFrozenTargetData* DataPtr = FrozenTargetMap.Find(TargetRef))
		{
			GetWorld()->GetTimerManager().ClearTimer(DataPtr->UnfreezeTimerHandle);
		}
		UnfreezeTarget(TargetRef);
	}

	FrozenTargetMap.Empty();
}