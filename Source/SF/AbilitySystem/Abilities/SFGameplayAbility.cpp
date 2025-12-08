#include "SFGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/SFCharacterBase.h"

USFGameplayAbility::USFGameplayAbility(const FObjectInitializer& ObjectInitializer)
{
	ActivationPolicy = ESFAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

USFAbilitySystemComponent* USFGameplayAbility::GetSFAbilitySystemComponentFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<USFAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr);
}

ASFCharacterBase* USFGameplayAbility::GetSFCharacterFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<ASFCharacterBase>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

ETeamAttitude::Type USFGameplayAbility::GetAttitudeTowards(AActor* Target) const
{
	if (!Target)
		return ETeamAttitude::Neutral;
	AActor* SourceActor = GetAvatarActorFromActorInfo();
	if (!SourceActor)
		return ETeamAttitude::Neutral;

	// Source의 TeamAgent 가져오기
	IGenericTeamAgentInterface* SourceTeamAgent = Cast<IGenericTeamAgentInterface>(SourceActor);
	if (!SourceTeamAgent)
		return ETeamAttitude::Neutral;
	// Target의 TeamAgent 가져오기
	IGenericTeamAgentInterface* TargetTeamAgent = Cast<IGenericTeamAgentInterface>(Target);
	if (!TargetTeamAgent)
		return ETeamAttitude::Neutral;
	// 적대 관계 확인
	ETeamAttitude::Type Attitude = FGenericTeamId::GetAttitude(
		SourceTeamAgent->GetGenericTeamId(),
		TargetTeamAgent->GetGenericTeamId()
	);
	return Attitude;
}

void USFGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	TryActivateAbilityOnSpawn(ActorInfo, Spec);
}

AController* USFGameplayAbility::GetControllerFromActorInfo() const
{
	if (CurrentActorInfo)
	{
		if (AController* PC = CurrentActorInfo->PlayerController.Get())
		{
			return PC;
		}

		// Look for a player controller or pawn in the owner chain.
		AActor* TestActor = CurrentActorInfo->OwnerActor.Get();
		while (TestActor)
		{
			if (AController* C = Cast<AController>(TestActor))
			{
				return C;
			}

			if (APawn* Pawn = Cast<APawn>(TestActor))
			{
				return Pawn->GetController();
			}

			TestActor = TestActor->GetOwner();
		}
	}

	return nullptr;
}

void USFGameplayAbility::TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const
{
	// Try to activate if activation policy is on spawn.
	if (ActorInfo && !Spec.IsActive() && (ActivationPolicy == ESFAbilityActivationPolicy::OnSpawn))
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		const AActor* AvatarActor = ActorInfo->AvatarActor.Get();

		// If avatar actor is torn off or about to die, don't try to activate until we get the new one.
		if (ASC && AvatarActor && !AvatarActor->GetTearOff() && (AvatarActor->GetLifeSpan() <= 0.0f))
		{
			const bool bIsLocalExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted) || (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalOnly);
			const bool bIsServerExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly) || (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerInitiated);

			const bool bClientShouldActivate = ActorInfo->IsLocallyControlled() && bIsLocalExecution;
			const bool bServerShouldActivate = ActorInfo->IsNetAuthority() && bIsServerExecution;

			if (bClientShouldActivate || bServerShouldActivate)
			{
				ASC->TryActivateAbility(Spec.Handle);
			}
		}
	}
}
