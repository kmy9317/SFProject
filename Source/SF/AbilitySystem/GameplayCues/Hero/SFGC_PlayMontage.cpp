// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGC_PlayMontage.h"

#include "AbilitySystem/GameplayCues/SFGameplayCueTags.h"
#include "Character/SFCharacterBase.h"

USFGC_PlayMontage::USFGC_PlayMontage()
{
	GameplayCueTag = SFGameplayTags::GameplayCue_Animation_PlayMontage;
}

bool USFGC_PlayMontage::HandlesEvent(EGameplayCueEvent::Type EventType) const
{
	return EventType == EGameplayCueEvent::Executed;
}

void USFGC_PlayMontage::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	if (EventType != EGameplayCueEvent::Executed)
	{
		return;
	}

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(MyTarget);
	if (!Character)
	{
		return;
	}

	const UAnimMontage* ConstMontage = Cast<UAnimMontage>(Parameters.SourceObject.Get());
	if (!ConstMontage)
	{
		return;
	}

	// PlayAnimMontage는 non-const를 요구하므로 const_cast 필요
	UAnimMontage* Montage = const_cast<UAnimMontage*>(ConstMontage);
	float PlayRate = (Parameters.RawMagnitude > 0.f) ? Parameters.RawMagnitude : 1.0f;
    
	Character->PlayAnimMontage(Montage, PlayRate);
}
