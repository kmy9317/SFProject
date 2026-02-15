#include "SFGA_Hero_Parrying.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AbilitySystem/GameplayCues/SFGameplayCueTags.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

USFGA_Hero_Parrying::USFGA_Hero_Parrying(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void USFGA_Hero_Parrying::ActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	//패링 성공 이벤트 대기
	if (HasAuthority(&ActivationInfo) && ParryEventTag.IsValid())
	{
		auto* Wait = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ParryEventTag);
		Wait->EventReceived.AddDynamic(this, &USFGA_Hero_Parrying::OnParryEventReceived);
		Wait->ReadyForActivation();
	}
}

void USFGA_Hero_Parrying::OnParryEventReceived(FGameplayEventData Payload)
{
	AActor* InstigatorActor = nullptr;
	if (!ValidateParryDirection(Payload, InstigatorActor))
	{
		return;
	}
	
	AActor* OwnerActor = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!OwnerActor || !OwnerASC)
	{
		return;
	}

	// 패링 성공 태그
	OwnerASC->AddLooseGameplayTag(ParryEventTag);

	// 공격자에게 Parried 이벤트 전달
	if (UAbilitySystemComponent* InstigatorASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InstigatorActor))
	{
		FGameplayEventData EventData;
		EventData.EventTag = SFGameplayTags::GameplayEvent_Parried;
		EventData.Instigator = OwnerActor;
		EventData.Target = InstigatorActor;
		EventData.ContextHandle = Payload.ContextHandle;
		InstigatorASC->HandleGameplayEvent(SFGameplayTags::GameplayEvent_Parried, &EventData);
	}

	if (!Payload.ContextHandle.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] ContextHandle INVALID"));
	}
	else if (const FHitResult* HR = Payload.ContextHandle.GetHitResult())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Parry] HitResult OK | Impact=%s | Bone=%s"), *HR->ImpactPoint.ToString(), *HR->BoneName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] ContextHandle VALID but HitResult NULL"));
	}

	ApplyComboState(this, ExecutingChainIndex + 1);

	// 패링 GC 실행
	if (!bParryGCExecuted)
	{
		FVector FXLocation = OwnerActor->GetActorLocation();
		if (Payload.ContextHandle.IsValid())
		{
			if (const FHitResult* Hit = Payload.ContextHandle.GetHitResult())
			{
				FXLocation = Hit->ImpactPoint;
			}
		}

		FVector DirToInstigator = (InstigatorActor->GetActorLocation() - OwnerActor->GetActorLocation()).GetSafeNormal();

		FGameplayCueParameters CueParams;
		CueParams.Location     = FXLocation;
		CueParams.Instigator   = OwnerActor;
		CueParams.EffectCauser = InstigatorActor;
		CueParams.SourceObject = this;
		CueParams.OriginalTag  = ParryEventTag;
		CueParams.Normal       = DirToInstigator;
		OwnerASC->ExecuteGameplayCue(SFGameplayTags::GameplayCue_Event_Parry, CueParams);

		bParryGCExecuted = true;
	}

	// 자식 클래스 확장 포인트
	OnParrySuccess(Payload, InstigatorActor);
}

bool USFGA_Hero_Parrying::ValidateParryDirection(const FGameplayEventData& Payload, AActor*& OutInstigator) const
{
	AActor* OwnerActor = GetAvatarActorFromActorInfo();
	if (!OwnerActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] OwnerActor NULL"));
		return false;
	}

	OutInstigator = Payload.ContextHandle.GetOriginalInstigator();
	if (!OutInstigator)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] Failed GetOriginalInstigator"));
		OutInstigator = const_cast<AActor*>(Payload.Instigator.Get());
	}

	if (!OutInstigator)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] Instigator NULL - Cannot calculate angle"));
		return false;
	}

	FVector OwnerLocation = OwnerActor->GetActorLocation();
	FVector InstigatorLocation = OutInstigator->GetActorLocation();
	FVector DirToInstigator = (InstigatorLocation - OwnerLocation).GetSafeNormal();
	FVector OwnerForward = OwnerActor->GetActorForwardVector().GetSafeNormal();

	float Dot = FVector::DotProduct(OwnerForward, DirToInstigator);
	float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));
	bool bInFront = (AngleDeg <= 90.0f);

	UE_LOG(LogTemp, Warning, TEXT("[Parry] Angle=%.2f | Dot=%.2f | InFront(180deg)=%d"), AngleDeg, Dot, bInFront);

	if (!bInFront)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] FAILED - Attack from BEHIND"));
		return false;
	}

	return true;
}

void USFGA_Hero_Parrying::OnParrySuccess(const FGameplayEventData& Payload, AActor* InstigatorActor)
{
	// Base는 빈 구현 - 자식 클래스에서 override하여 추가 로직 수행
}

void USFGA_Hero_Parrying::OnChainMontageCompleted()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	const int32 CurrentChainIndex = GetCurrentChain(); 
	const int32 LastChainIndex = GetChainConfigs().Num() - 1;
	
	if (!ASC->HasMatchingGameplayTag(ParryEventTag))
	{
		if (CurrentChainIndex < LastChainIndex)
		{
			// 실패 → 쿨타임 강제 적용
			CompleteCombo(this);
			UE_LOG(LogTemp, Warning, TEXT("[Parry] FAILED → Apply Complete Cooldown"));
            
			RemoveChainEffects();
			RestoreSlidingMode();
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			return;
		}
	}

	// 패링 성공 or 마지막 체인 → Super 호출
	Super::OnChainMontageCompleted();
}

void USFGA_Hero_Parrying::OnChainMontageInterrupted()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    
	const int32 CurrentChainIndex = GetCurrentChain(); 
	const int32 LastChainIndex = GetChainConfigs().Num() - 1;
	
	if (!ASC->HasMatchingGameplayTag(ParryEventTag))
	{
		if (CurrentChainIndex < LastChainIndex)
		{
			// 패링 실패 (인터럽트) → Complete 쿨다운
			CompleteCombo(this);
			UE_LOG(LogTemp, Warning, TEXT("[Parry] INTERRUPTED → Apply Complete Cooldown"));
            
			RemoveChainEffects();
			RestoreSlidingMode();
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
			return;
		}
	}

	Super::OnChainMontageInterrupted();
}

void USFGA_Hero_Parrying::EndAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,bool bReplicateEndAbility,bool bWasCancelled)
{
	bParryGCExecuted = false;
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	
	if (ASC && ASC->HasMatchingGameplayTag(ParryEventTag))
	{
		ASC->RemoveLooseGameplayTag(ParryEventTag);
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
