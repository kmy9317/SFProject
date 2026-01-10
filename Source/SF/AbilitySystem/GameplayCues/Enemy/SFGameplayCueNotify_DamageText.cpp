// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGameplayCueNotify_DamageText.h"

#include "GameFramework/PlayerState.h"
#include "System/SFDamageTextSubSystem.h"
#include "UI/InGame/UIDataStructs.h"

USFGameplayCueNotify_DamageText::USFGameplayCueNotify_DamageText()
{
}

bool USFGameplayCueNotify_DamageText::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!MyTarget) return false;
	
	APlayerController* LocalPC = GetWorld()->GetFirstPlayerController();
	if (!LocalPC || !LocalPC->IsLocalController()) return false;
	
	AActor* InstigatorActor = Parameters.Instigator.Get();
	if (!InstigatorActor) return false;
	
	AController* InstigatorController = nullptr;
	
	if (APawn* InstigatorPawn = Cast<APawn>(InstigatorActor))
	{
		InstigatorController = InstigatorPawn->GetController();
	}
	else if (APlayerState* InstigatorPS = Cast<APlayerState>(InstigatorActor))
	{
		InstigatorController = InstigatorPS->GetPlayerController();
	}
	
	else if (AController* PC = Cast<AController>(InstigatorActor))
	{
		InstigatorController = PC;
	}
	
	
	if (InstigatorController != LocalPC)
	{
		return false;
	}
	
	if (USFDamageTextSubSystem* DamageSystem = MyTarget->GetWorld()->GetSubsystem<USFDamageTextSubSystem>())
	{
		DamageSystem->ShowDamage(Parameters.RawMagnitude, MyTarget, Parameters.Location);
		return true;
	}

	return false; 
}