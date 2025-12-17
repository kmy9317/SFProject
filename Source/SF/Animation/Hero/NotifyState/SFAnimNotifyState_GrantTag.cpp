// Fill out your copyright notice in the Description page of Project Settings.


#include "SFAnimNotifyState_GrantTag.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

void USFAnimNotifyState_GrantTag::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp || !MeshComp->GetOwner())
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
    
	if (!ASC || !GameplayTag.IsValid())
	{
		return;
	}

	// 태그 부여
	ASC->AddLooseGameplayTag(GameplayTag);

	// 시작 이벤트 브로드캐스트
	if (bBroadcastEventOnBegin)
	{
		BroadcastGameplayEvent(ASC, Owner, GameplayTag);
	}
}

void USFAnimNotifyState_GrantTag::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp || !MeshComp->GetOwner())
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
    
	if (!ASC || !GameplayTag.IsValid())
	{
		return;
	}

	// 종료 이벤트 브로드캐스트 (태그 제거 전에 보냄)
	if (bBroadcastEventOnEnd)
	{
		FGameplayTag TagToSend = EndEventTag.IsValid() ? EndEventTag : GameplayTag;
		BroadcastGameplayEvent(ASC, Owner, TagToSend);
	}

	// 태그 제거
	ASC->RemoveLooseGameplayTag(GameplayTag);
}

void USFAnimNotifyState_GrantTag::BroadcastGameplayEvent(UAbilitySystemComponent* ASC, AActor* Owner, const FGameplayTag& EventTag)
{
	FGameplayEventData EventData;
	EventData.EventTag = EventTag;
	EventData.Instigator = Owner;
	EventData.Target = Owner;
	ASC->HandleGameplayEvent(EventTag, &EventData);
}
