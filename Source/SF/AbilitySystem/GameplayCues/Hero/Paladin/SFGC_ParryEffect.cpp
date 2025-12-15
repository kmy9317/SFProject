#include "SFGC_ParryEffect.h"

#include "Kismet/GameplayStatics.h"

bool USFGC_ParryEffect::OnExecute_Implementation(
	AActor* Target,
	const FGameplayCueParameters& Parameters
) const
{
	if (!Target)
	{
		return false;
	}

	//==================== Base Location ====================
	FVector SpawnLocation = Parameters.Location;
	SpawnLocation += LocationOffset;

	if (bUseForwardOffset)
	{
		SpawnLocation += Target->GetActorForwardVector() * ForwardOffset;
	}
	//======================================================

	//==================== Rotation =========================
	const FRotator SpawnRotation =
		Parameters.Normal.IsNearlyZero()
		? Target->GetActorRotation()
		: Parameters.Normal.Rotation();
	//======================================================

	//==================== FX (Cascade) =====================
	if (ParryFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			Target->GetWorld(),
			ParryFX,
			SpawnLocation,
			SpawnRotation,
			FVector(ParticleScale), // ★ 크기 조절 핵심
			true
		);
	}
	//======================================================

	//==================== Sound =============================
	if (ParrySound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			Target,
			ParrySound,
			SpawnLocation
		);
	}
	//======================================================

	return true;
}
