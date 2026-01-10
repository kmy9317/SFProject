#include "SFGC_Hero_AreaHeal_C_Lightning.h"

#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
// [추가] 나이아가라 관련 헤더
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include "Sound/SoundBase.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFGC_Hero_AreaHeal_C_Lightning)

//=====================스폰 위치 계산=====================
FVector USFGC_Hero_AreaHeal_C_Lightning::ResolveSpawnLocation(
	AActor* Target,
	const FGameplayCueParameters& Parameters) const
{
	if (!Parameters.Location.IsNearlyZero()) return Parameters.Location; //GameplayCue에서 Location 직접 전달된 경우

	if (bUseTargetFloorIfNoLocation && Target) //없으면 발밑 라인추적
	{
		UWorld* World = Target->GetWorld();
		if (!World) return Target->GetActorLocation();

		FHitResult Hit;
		FVector Start = Target->GetActorLocation() + FVector(0,0,FloorTraceUp);
		FVector End   = Target->GetActorLocation() - FVector(0,0,FloorTraceDown);

		FCollisionQueryParams Params(SCENE_QUERY_STAT(SFGC_AreaHeal_C_FloorTrace),false,Target);

		if (World->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Params))
			return Hit.ImpactPoint + FVector(0,0,5.f); //지면 약간 위

		return Target->GetActorLocation(); //Trace 실패
	}

	return Target ? Target->GetActorLocation() : FVector::ZeroVector; //fallback
}
//============================================================


//=====================GameplayCue 실행=====================
void USFGC_Hero_AreaHeal_C_Lightning::HandleGameplayCue(
	AActor* Target,
	EGameplayCueEvent::Type EventType,
	const FGameplayCueParameters& Parameters)
{
	if (EventType!=EGameplayCueEvent::Executed && EventType!=EGameplayCueEvent::OnActive) return; //원샷 전용
	if (!Target) return;

	UWorld* World = Target->GetWorld();
	if (!World) return;

	const FVector SpawnLocation = ResolveSpawnLocation(Target,Parameters);
	const FRotator SpawnRotation = FRotator::ZeroRotator;

	//=====================Cascade 파티클=====================
	if (LightningEffect1)
	{
		FTransform T(SpawnRotation,SpawnLocation,ParticleScale);
		UGameplayStatics::SpawnEmitterAtLocation(World,LightningEffect1,T,true);
	}

	if (LightningEffect2)
	{
		FTransform T(SpawnRotation,SpawnLocation,ParticleScale);
		UGameplayStatics::SpawnEmitterAtLocation(World,LightningEffect2,T,true);
	}

	if (LightningEffect3)
	{
		FTransform T(SpawnRotation,SpawnLocation,ParticleScale);
		UGameplayStatics::SpawnEmitterAtLocation(World,LightningEffect3,T,true);
	}
	//========================================================

	//=====================Niagara 파티클 [추가]===============
	// 나이아가라 시스템은 UNiagaraFunctionLibrary를 사용하여 스폰합니다.
	if (LightningNiagara1)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, 
			LightningNiagara1, 
			SpawnLocation, 
			SpawnRotation, 
			ParticleScale, 
			true, 
			true, 
			ENCPoolMethod::AutoRelease);
	}

	if (LightningNiagara2)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, 
			LightningNiagara2, 
			SpawnLocation, 
			SpawnRotation, 
			ParticleScale, 
			true, 
			true, 
			ENCPoolMethod::AutoRelease);
	}
	//========================================================

	//=====================사운드=====================
	if (LightningSound1)
	{
		UGameplayStatics::PlaySoundAtLocation(
			World,LightningSound1,SpawnLocation,VolumeMultiplier,PitchMultiplier);
	}

	if (LightningSound2)
	{
		UGameplayStatics::PlaySoundAtLocation(
			World,LightningSound2,SpawnLocation,VolumeMultiplier,PitchMultiplier);
	}
	//================================================
}
//============================================================