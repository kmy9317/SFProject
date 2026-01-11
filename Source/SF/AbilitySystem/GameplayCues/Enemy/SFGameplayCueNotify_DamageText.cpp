// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGameplayCueNotify_DamageText.h"

#include "AbilitySystem/GameplayEffect/SFGameplayEffectContext.h"
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


	FGameplayEffectContextHandle ContextHandle = Parameters.EffectContext;
	if (!ContextHandle.IsValid()) return false;
	
	AActor* InstigatorActor = ContextHandle.GetInstigator();
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

	FVector HitLocation = FVector::ZeroVector;
	if (const FHitResult* HitResult = ContextHandle.GetHitResult())
	{
		HitLocation = HitResult->ImpactPoint;
	}
	
	if (USFDamageTextSubSystem* DamageSystem = MyTarget->GetWorld()->GetSubsystem<USFDamageTextSubSystem>())
	{
		bool bIsCritical = false;
		if (FSFGameplayEffectContext* SFContext = static_cast<FSFGameplayEffectContext*>(ContextHandle.Get()))
		{
			bIsCritical = SFContext->IsCriticalHit();
		}
		DamageSystem->ShowDamage(Parameters.RawMagnitude, MyTarget, HitLocation, bIsCritical);
		return true;
	}

	return false; 
}