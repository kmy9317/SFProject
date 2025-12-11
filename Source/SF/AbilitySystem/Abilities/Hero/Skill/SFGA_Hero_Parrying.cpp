#include "SFGA_Hero_Parrying.h"

#include "AbilitySystemComponent.h"
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

	// 4) 후방 공격 → 패링 실패
	if (!bInFront)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] FAILED - Attack from BEHIND"));
		return;
	}

	// 5) 패링 성공
	UE_LOG(LogTemp, Error, TEXT("[Parry] SUCCESS - Attack from FRONT (180 deg)"));

	if (HasAuthority(&CurrentActivationInfo))
	{
		GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(ParryEventTag);
	}
	
	ApplyComboState(this, ExecutingChainIndex + 1);
	
	//Parried 이벤트 전송
}

void USFGA_Hero_Parrying::OnChainMontageCompleted()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    
	bool bSuccess = ASC->HasMatchingGameplayTag(ParryEventTag);

	if (!bSuccess)
	{
		// 실패 → 쿨타임 강제 적용
		ApplyCooldown(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
		UE_LOG(LogTemp, Warning, TEXT("[Parry] FAILED → Apply Cooldown"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Parry] SUCCESS → No cooldown"));
	}

	Super::OnChainMontageCompleted();
}

void USFGA_Hero_Parrying::OnChainMontageInterrupted()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	bool bSuccess = ASC->HasMatchingGameplayTag(ParryEventTag);

	if (!bSuccess)
	{
		ApplyCooldown(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
		UE_LOG(LogTemp, Warning, TEXT("[Parry] INTERRUPTED → Apply Cooldown"));
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
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	
	if (ASC && ASC->HasMatchingGameplayTag(ParryEventTag))
	{
		ASC->RemoveLooseGameplayTag(ParryEventTag);
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
