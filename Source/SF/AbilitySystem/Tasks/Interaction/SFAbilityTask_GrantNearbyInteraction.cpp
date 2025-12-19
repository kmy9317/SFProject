// Fill out your copyright notice in the Description page of Project Settings.


#include "SFAbilityTask_GrantNearbyInteraction.h"

#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "Interaction/SFInteractable.h"
#include "Physics/SFCollisionChannels.h"

USFAbilityTask_GrantNearbyInteraction* USFAbilityTask_GrantNearbyInteraction::GrantAbilitiesForNearbyInteractables(UGameplayAbility* OwningAbility, float InteractionAbilityScanRange, float InteractionAbilityScanRate)
{
	USFAbilityTask_GrantNearbyInteraction* Task = NewAbilityTask<USFAbilityTask_GrantNearbyInteraction>(OwningAbility);
	Task->InteractionAbilityScanRange = InteractionAbilityScanRange;
	Task->InteractionAbilityScanRate = InteractionAbilityScanRate;
	return Task;
}

void USFAbilityTask_GrantNearbyInteraction::Activate()
{
	Super::Activate();

	SetWaitingOnAvatar();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(QueryTimerHandle, this, &ThisClass::QueryInteractables, InteractionAbilityScanRate, true);
	}
}

void USFAbilityTask_GrantNearbyInteraction::OnDestroy(bool bInOwnerFinished)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(QueryTimerHandle);
	}
	
	Super::OnDestroy(bInOwnerFinished);
}

void USFAbilityTask_GrantNearbyInteraction::QueryInteractables()
{
	UWorld* World = GetWorld();
	AActor* AvatarActor = GetAvatarActor();
	
	if (World && AvatarActor)
	{
		TSet<FObjectKey> RemoveKeys;
		GrantedInteractionAbilities.GetKeys(RemoveKeys);

		// 해당 테스크에 대한 성능 검사
		FCollisionQueryParams Params(SCENE_QUERY_STAT(USFAbilityTask_GrantNearbyInteraction), false);

		TArray<FOverlapResult> OverlapResults;
		World->OverlapMultiByChannel(OverlapResults, AvatarActor->GetActorLocation(), FQuat::Identity, SF_TraceChannel_Interaction, FCollisionShape::MakeSphere(InteractionAbilityScanRange), Params);
		
		if (OverlapResults.Num() > 0)
		{
			// 감지된 객체들을 상호작용 가능한 객체로 필터링
			TArray<TScriptInterface<ISFInteractable>> Interactables;
			for (const FOverlapResult& OverlapResult : OverlapResults)
			{
				TScriptInterface<ISFInteractable> InteractableActor(OverlapResult.GetActor());
				if (InteractableActor)
				{
					Interactables.AddUnique(InteractableActor);
				}
		
				TScriptInterface<ISFInteractable> InteractableComponent(OverlapResult.GetComponent());
				if (InteractableComponent)
				{
					Interactables.AddUnique(InteractableComponent);
				}
			}

			// 각 상호작용 객체로부터 상호작용 정보 수집
			FSFInteractionQuery InteractionQuery;
			InteractionQuery.RequestingAvatar = AvatarActor;
			InteractionQuery.RequestingController = Cast<AController>(AvatarActor->GetOwner());
		
			TArray<FSFInteractionInfo> InteractionInfos;
			for (TScriptInterface<ISFInteractable>& Interactable : Interactables)
			{
				FSFInteractionInfoBuilder InteractionInfoBuilder(Interactable, InteractionInfos);
				Interactable->GatherPostInteractionInfos(InteractionQuery, InteractionInfoBuilder);
			}
		
			for (FSFInteractionInfo& InteractionInfo : InteractionInfos)
			{
				if (InteractionInfo.AbilityToGrant)
				{
					FObjectKey ObjectKey(InteractionInfo.AbilityToGrant);
					// 현재 감지된 객체가 주는 Ability를 이미 부여받았는지 확인
					if (GrantedInteractionAbilities.Find(ObjectKey))
					{
						RemoveKeys.Remove(ObjectKey);
					}
					// 새로 받는 Interaction 타입의 Ability인 경우 부여
					else
					{
						FGameplayAbilitySpec Spec(InteractionInfo.AbilityToGrant, 1, INDEX_NONE, this);
						FGameplayAbilitySpecHandle SpecHandle = AbilitySystemComponent->GiveAbility(Spec);
						GrantedInteractionAbilities.Add(ObjectKey, SpecHandle);
					}
				}
			}
		}
		
		// 감지되지 않은 객체 타입의 Interaction Ability는 제거
		for (const FObjectKey& RemoveKey : RemoveKeys)
		{
			AbilitySystemComponent->ClearAbility(GrantedInteractionAbilities[RemoveKey]);
			GrantedInteractionAbilities.Remove(RemoveKey);
		}
	}
}
