#include "SFGCN_HeartBreaker_Charging.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/SFCharacterBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Weapons/Actor/SFEquipmentBase.h"

ASFGCN_HeartBreaker_Charging::ASFGCN_HeartBreaker_Charging()
{
	
}

bool ASFGCN_HeartBreaker_Charging::WhileActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	if (ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(Target))
	{
		CachedTargetCharacter = SFCharacter;
	}

	if (ASFEquipmentBase* SFEquipment = Cast<ASFEquipmentBase>(Parameters.GetEffectCauser()))
	{
		CachedTargetWeapon = SFEquipment;
	}

	bIsLocalPlayer = IsLocallyControlled(Target);
	
	CurrentChargePhase = FMath::RoundToInt(Parameters.RawMagnitude);

	// WeaponAura 스폰
	if (WeaponAuraSystem && CachedTargetWeapon.IsValid() && !WeaponAuraComponent)
	{
		WeaponAuraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			WeaponAuraSystem,
			CachedTargetWeapon->GetMeshComponent(),
			WeaponAuraSocket,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			false,
			true);
	}

	// BodyMeshCharging 스폰
	if (BodyMeshChargingSystem && CachedTargetCharacter.IsValid() && !BodyMeshChargingComponent)
	{
		BodyMeshChargingComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			BodyMeshChargingSystem,
			CachedTargetCharacter->GetMesh(),
			BodyChargingSocket,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			false,
			true);
	}

	if (bIsLocalPlayer)
	{
		StartChargingSound();
		PlayPhaseTransitionSound(CurrentChargePhase);
	}
	
	UpdateNiagaraPhaseParameters(CurrentChargePhase);
	
	return false;
}

bool ASFGCN_HeartBreaker_Charging::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	int32 NewPhase = FMath::RoundToInt(Parameters.RawMagnitude);
	if (CurrentChargePhase != NewPhase)
	{
		CurrentChargePhase = NewPhase;

		UpdateNiagaraPhaseParameters(CurrentChargePhase);

		if (bIsLocalPlayer)
		{
			PlayPhaseTransitionSound(CurrentChargePhase);
		}
	}
	
	return false;
}

bool ASFGCN_HeartBreaker_Charging::OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	CachedTargetCharacter = nullptr;
	CachedTargetWeapon = nullptr;

	StopChargingSound();
	
	if (WeaponAuraComponent)
	{
		WeaponAuraComponent->DestroyComponent();
		WeaponAuraComponent = nullptr;
	}

	if (BodyMeshChargingComponent)
	{
		BodyMeshChargingComponent->DestroyComponent();
		BodyMeshChargingComponent = nullptr;
	}

	bIsLocalPlayer = false;
	CurrentChargePhase = 0;
	
	return false;
}

void ASFGCN_HeartBreaker_Charging::UpdateNiagaraPhaseParameters(int32 Phase)
{
	if (BodyMeshChargingComponent && BodyMeshChargingComponent->IsActive())
	{
		if (BodyPhaseParams.IsValidIndex(Phase))
		{
			const FSFBodyChargingPhaseParams& Params = BodyPhaseParams[Phase];
			BodyMeshChargingComponent->SetVariableBool(BodyParam_AddDetail, Params.bAddDetail);
			BodyMeshChargingComponent->SetVariableBool(BodyParam_Simple, Params.bSimple);
			BodyMeshChargingComponent->SetVariableLinearColor(BodyParam_ColorTint, Params.ColorTint);
			BodyMeshChargingComponent->SetVariableFloat(BodyParam_Size, Params.Size);
		}
	}

	if (WeaponAuraComponent && WeaponAuraComponent->IsActive())
	{
		if (WeaponAuraParams.IsValidIndex(Phase))
		{
			const FSFWeaponAuraPhaseParams& Params = WeaponAuraParams[Phase];
			WeaponAuraComponent->SetVariableLinearColor(TEXT("TintColor"), Params.ColorTint);
		}
	}
}

bool ASFGCN_HeartBreaker_Charging::IsLocallyControlled(AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	// Pawn인 경우 IsLocallyControlled 체크
	if (APawn* Pawn = Cast<APawn>(Target))
	{
		return Pawn->IsLocallyControlled();
	}

	return false;
}

void ASFGCN_HeartBreaker_Charging::StartChargingSound()
{
	if (!ChargingLoopSound || ChargingAudioComponent)
	{
		return;
	}

	if (!CachedTargetCharacter.IsValid())
	{
		return;
	}

	ChargingAudioComponent = UGameplayStatics::SpawnSound2D(
		CachedTargetCharacter.Get(),
		ChargingLoopSound,
		ChargingLoopVolume,
		ChargingLoopPitch,
		0.0f,
		nullptr,
		false,
		false
	);
}

void ASFGCN_HeartBreaker_Charging::StopChargingSound()
{
	if (ChargingAudioComponent)
	{
		ChargingAudioComponent->Stop();
		ChargingAudioComponent->DestroyComponent();
		ChargingAudioComponent = nullptr;
	}
}

void ASFGCN_HeartBreaker_Charging::PlayPhaseTransitionSound(int32 Phase)
{
	if (!PhaseTransitionSounds.IsValidIndex(Phase))
	{
		return;
	}

	USoundBase* TransitionSound = PhaseTransitionSounds[Phase];
	if (!TransitionSound)
	{
		return;
	}

	if (!CachedTargetCharacter.IsValid())
	{
		return;
	}

	UGameplayStatics::PlaySound2D(
		CachedTargetCharacter.Get(),
		TransitionSound,
		PhaseTransitionVolume,
		PhaseTransitionPitch,
		0.0f
	);
}
