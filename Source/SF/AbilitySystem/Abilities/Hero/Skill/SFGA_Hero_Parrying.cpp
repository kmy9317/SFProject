#include "SFGA_Hero_Parrying.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AbilitySystem/GameplayCues/SFGameplayCueTags.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

//=====================================================================
// Constructor
//=====================================================================
USFGA_Hero_Parrying::USFGA_Hero_Parrying(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}


//=====================================================================
// ActivateAbility
//=====================================================================
void USFGA_Hero_Parrying::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
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


//=====================================================================
void USFGA_Hero_Parrying::OnParryEventReceived(FGameplayEventData Payload)
{
	AActor* OwnerActor = GetAvatarActorFromActorInfo();
	if (!OwnerActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] OwnerActor NULL"));
		return;
	}

	// 1) 공격자 찾기
	AActor* InstigatorActor = Payload.ContextHandle.GetOriginalInstigator();
	if (!InstigatorActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] Failed GetOriginalInstigator"));
		InstigatorActor = const_cast<AActor*>(Payload.Instigator.Get());
	}

	if (!InstigatorActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] Instigator NULL - Cannot calculate angle"));
		return;
	}

	//방향 계산
	FVector OwnerLocation = OwnerActor->GetActorLocation();
	FVector InstigatorLocation = InstigatorActor->GetActorLocation();
	FVector DirToInstigator = (InstigatorLocation - OwnerLocation).GetSafeNormal();

	FVector OwnerForward = OwnerActor->GetActorForwardVector().GetSafeNormal();

	//각도 계산
	float Dot = FVector::DotProduct(OwnerForward, DirToInstigator);
	float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));

	//전방 180° 에서 공격이 들어왔는지 판단
	bool bInFront = (AngleDeg <= 90.0f);

	UE_LOG(LogTemp, Warning, TEXT("[Parry] Angle=%.2f | Dot=%.2f | InFront(180deg)=%d"),
		AngleDeg, Dot, bInFront);

	//후방 공격 → 패링 실패
	if (!bInFront)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] FAILED - Attack from BEHIND"));
		return;
	}

	//패링 성공 로그
	UE_LOG(LogTemp, Error, TEXT("[Parry] SUCCESS - Attack from FRONT (180 deg)"));

	//패링 이벤트 전달 및 GC로 패링 이펙트 처리
	if (HasAuthority(&CurrentActivationInfo))
	{
		UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
		if (!OwnerASC) return;

		//패링 성공 태그
		OwnerASC->AddLooseGameplayTag(ParryEventTag);

		//공격자에게 Parried 이벤트 전달
		if (UAbilitySystemComponent* InstigatorASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InstigatorActor))
		{
			FGameplayEventData EventData;
			EventData.EventTag      = SFGameplayTags::GameplayEvent_Parried;
			EventData.Instigator    = OwnerActor;
			EventData.Target        = InstigatorActor;
			EventData.ContextHandle = Payload.ContextHandle;

			InstigatorASC->HandleGameplayEvent(
				SFGameplayTags::GameplayEvent_Parried,
				&EventData
			);
		}

		if (!Payload.ContextHandle.IsValid())
		{
			UE_LOG(LogTemp, Error,
				TEXT("[Parry] ContextHandle INVALID"));
		}
		else if (const FHitResult* HR = Payload.ContextHandle.GetHitResult())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Parry] HitResult OK | Impact=%s | Bone=%s"),
				*HR->ImpactPoint.ToString(),
				*HR->BoneName.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Error,
				TEXT("[Parry] ContextHandle VALID but HitResult NULL"));
		}

		ApplyComboState(this, ExecutingChainIndex + 1);
		
		// 이미 패링 GC 실행됐으면 무시
		if (bParryGCExecuted)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Parry] GC already executed - Skip"));
			return;
		}
		
		//패링 GameplayCue 실행
		FVector FXLocation = OwnerLocation;
		if (Payload.ContextHandle.IsValid())
		{
			if (const FHitResult* Hit = Payload.ContextHandle.GetHitResult())
			{
				FXLocation = Hit->ImpactPoint;
			}
		}

		FGameplayCueParameters CueParams;
		CueParams.Location     = FXLocation;
		CueParams.Instigator   = OwnerActor;
		CueParams.EffectCauser = InstigatorActor;
		CueParams.SourceObject = this;
		CueParams.OriginalTag  = ParryEventTag;
		CueParams.Normal       = DirToInstigator;

		OwnerASC->ExecuteGameplayCue(
			SFGameplayTags::GameplayCue_Event_Parry,
			CueParams
			);
		
		//GC 실행 완료 표시
		bParryGCExecuted = true;
	}
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
//=====================================================================



//=====================================================================
// EndAbility
//=====================================================================
void USFGA_Hero_Parrying::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bParryGCExecuted = false;
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	
	if (ASC && ASC->HasMatchingGameplayTag(ParryEventTag))
	{
		ASC->RemoveLooseGameplayTag(ParryEventTag);
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
