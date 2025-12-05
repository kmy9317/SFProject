// Fill out your copyright notice in the Description page of Project Settings.


#include "SFAnimNotifyState_GrantTag.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

void USFAnimNotifyState_GrantTag::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		// 1. ASC를 가져옴.
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner());
		if (ASC && GameplayTag.IsValid())
		{
			// 2. 태그를 추가. (콤보 입력 유효 상태)
			// LooseTag는 ASC에 일시적으로 부여하는 태그.
			ASC->AddLooseGameplayTag(GameplayTag);
		}
	}
}

void USFAnimNotifyState_GrantTag::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner());
		if (ASC && GameplayTag.IsValid())
		{
			// 3. 태그 제거
			ASC->RemoveLooseGameplayTag(GameplayTag);
		}
	}
}