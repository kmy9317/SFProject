#include "SFGC_WeaponBurst.h"

#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "AbilitySystem/GameplayCues/Data/SFDA_WeaponBurstData.h" // 데이터 에셋 헤더

// 프로젝트의 장비 클래스 헤더
#include "NiagaraComponent.h"
#include "Weapons/Actor/SFEquipmentBase.h"

bool USFGC_WeaponBurst::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const
{
	if (!IsValid(Target))
	{
		return false;
	}

	// 1. 설정값 초기화 (기본값)
	UNiagaraSystem* NiagaraToSpawn = DefaultNiagaraVFX;
	UParticleSystem* CascadeToSpawn = nullptr;
	USoundBase* SoundToPlay = DefaultSound;
	FName SocketName = DefaultSocketName;
	bool bAttach = true;
	FVector Scale = FVector(1.f);
	float VolMult = 1.f;
	float PitchMult = 1.f;

	// 2. DataAsset이 Payload(SourceObject)로 넘어왔는지 확인 및 덮어쓰기
	if (const USFDA_WeaponBurstData* Data = Cast<USFDA_WeaponBurstData>(Parameters.SourceObject.Get()))
	{
		if (Data->NiagaraVFX) NiagaraToSpawn = Data->NiagaraVFX;
		if (Data->CascadeVFX) CascadeToSpawn = Data->CascadeVFX;
		if (Data->Sound) SoundToPlay = Data->Sound;
		
		SocketName = Data->SocketName;
		bAttach = Data->bAttachToSocket;
		Scale = Data->Scale;
		VolMult = Data->VolumeMultiplier;
		PitchMult = Data->PitchMultiplier;
	}

	// 3. 이펙트를 재생할 무기 메쉬 찾기
	USceneComponent* WeaponMesh = nullptr;

	// Case A: Target이 무기 자체인 경우
	if (ASFEquipmentBase* Equipment = Cast<ASFEquipmentBase>(Target))
	{
		WeaponMesh = Equipment->GetMeshComponent();
	}
	// Case B: Target이 캐릭터인 경우 (장착된 무기 검색)
	else
	{
		TArray<AActor*> AttachedActors;
		Target->GetAttachedActors(AttachedActors);

		for (AActor* AttachedActor : AttachedActors)
		{
			if (ASFEquipmentBase* FoundEquipment = Cast<ASFEquipmentBase>(AttachedActor))
			{
				WeaponMesh = FoundEquipment->GetMeshComponent();
				break; // 첫 번째 무기 사용 (필요시 로직 고도화 가능)
			}
		}

		// 무기를 못 찾으면 타겟 자체(캐릭터)를 기준으로
		if (!WeaponMesh)
		{
			WeaponMesh = Target->GetRootComponent();
		}
	}

	if (!WeaponMesh)
	{
		return false;
	}

	// 4. 위치 계산
	FVector SpawnLoc = WeaponMesh->GetSocketLocation(SocketName);
	FRotator SpawnRot = WeaponMesh->GetSocketRotation(SocketName);

	UWorld* World = Target->GetWorld();

	// 5. Niagara 재생
	if (NiagaraToSpawn)
	{
		if (bAttach)
		{
			UNiagaraComponent* SpawnedSystem = UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraToSpawn,
			WeaponMesh,
			SocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
			);
			
			// 생성된 컴포넌트가 유효한지 확인 후 스케일 조절
			if (SpawnedSystem)
			{
				SpawnedSystem->SetRelativeScale3D(Scale);
			}
		}
		else
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World,
				NiagaraToSpawn,
				SpawnLoc,
				SpawnRot,
				Scale
			);
		}
	}

	// 6. Cascade 재생 (레거시 지원)
	if (CascadeToSpawn)
	{
		if (bAttach)
		{
			UGameplayStatics::SpawnEmitterAttached(
				CascadeToSpawn,
				WeaponMesh,
				SocketName,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				Scale,
				EAttachLocation::SnapToTarget,
				true
			);
		}
		else
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				CascadeToSpawn,
				SpawnLoc,
				SpawnRot,
				Scale
			);
		}
	}

	// 7. 사운드 재생
	if (SoundToPlay)
	{
		UGameplayStatics::PlaySoundAtLocation(
			World,
			SoundToPlay,
			SpawnLoc,
			VolMult,
			PitchMult
		);
	}

	return true;
}