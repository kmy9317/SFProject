// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGameplayCueNotify_DamageText.h"

#include "System/SFDamageTextSubSystem.h"
#include "UI/InGame/UIDataStructs.h"

USFGameplayCueNotify_DamageText::USFGameplayCueNotify_DamageText()
{
}

bool USFGameplayCueNotify_DamageText::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (MyTarget)
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (!PC || !PC->IsLocalController()) return false;
        
		APawn* LocalPawn = PC->GetPawn();
		if (!LocalPawn)
		{
			return false;
		}
        
		if (Parameters.Instigator != LocalPawn)
		{

			return false;
		}
		if (USFDamageTextSubSystem* DamageSystem = MyTarget->GetWorld()->GetSubsystem<USFDamageTextSubSystem>())
		{
			DamageSystem->ShowDamage(Parameters.RawMagnitude, MyTarget, Parameters.Location);
			return true;
		}
	}
	return false; 

}