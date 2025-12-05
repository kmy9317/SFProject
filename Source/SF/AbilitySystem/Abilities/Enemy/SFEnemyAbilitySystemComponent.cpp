// Fill out your copyright notice in the Description page of Project Settings.


#include "SFEnemyAbilitySystemComponent.h"

#include "AbilitySystem/GameplayEffect/SFGameplayEffectTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Animation/Enemy/SFEnemyAnimInstance.h"





void USFEnemyAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
	check(ActorInfo);
	check(InOwnerActor);

	// 새로운 Pawn Avatar가 설정되었는지 확인
	const bool bHasNewPawnAvatar = Cast<APawn>(InAvatarActor) && (InAvatarActor != ActorInfo->AvatarActor);
	
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (bHasNewPawnAvatar)
	{
		
		TryActivateAbilitiesOnSpawn();
		USFEnemyAnimInstance* AniminInstance = Cast<USFEnemyAnimInstance>(GetAvatarActor()->FindComponentByClass<USkeletalMeshComponent>()->GetAnimInstance());
		if (IsValid(AniminInstance))
		{
			AniminInstance->InitializeWithAbilitySystem(this);
		}
		OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &USFEnemyAbilitySystemComponent::HandleGameplayEffectAppliedToSelf);
	}
}

void USFEnemyAbilitySystemComponent::HandleGameplayEffectAppliedToSelf(
	UAbilitySystemComponent* SourceASC,
	const FGameplayEffectSpec& Spec,
	FActiveGameplayEffectHandle Handle)
{
	if (!Spec.Def)
		return;

	// Damage가 0이면 스킵
	float DamageValue = Spec.GetSetByCallerMagnitude(
		SFGameplayTags::Data_Damage_BaseDamage, false, 0.f);

	if (DamageValue <= 0.f)
		return;

	// GameplayEvent 발송
	FGameplayEventData Payload;
	Payload.EventTag = SFGameplayTags::GameplayEvent_HitReaction;
	Payload.Target = GetAvatarActor();
	Payload.Instigator = Spec.GetContext().GetOriginalInstigator();
	Payload.ContextHandle = Spec.GetContext();
	Payload.EventMagnitude = DamageValue;
	
	HandleGameplayEvent( SFGameplayTags::GameplayEvent_HitReaction, &Payload);
}


