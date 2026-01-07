#include "SFGCN_BuffAura.h"

#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
// [추가됨] 캐스케이드 컴포넌트 헤더
#include "Particles/ParticleSystemComponent.h" 
#include "AbilitySystem/GameplayCues/Data/SFDA_BuffAuraEffectData.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

ASFGCN_BuffAura::ASFGCN_BuffAura()
{
	bAutoDestroyOnRemove = true;
}

bool ASFGCN_BuffAura::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (!MyTarget)
	{
		return false;
	}

	// 중복 생성 방지 (나이아가라 혹은 캐스케이드 중 하나라도 살아있으면)
	bool bNiagaraActive = SpawnedNiagaraComponent && SpawnedNiagaraComponent->IsActive();
	bool bCascadeActive = SpawnedCascadeComponent && SpawnedCascadeComponent->IsActive();

	if (bNiagaraActive || bCascadeActive)
	{
		return false;
	}

	// 1. 기본값 설정
	UNiagaraSystem* NiagaraToSpawn = DefaultNiagaraSystem;
	UParticleSystem* CascadeToSpawn = DefaultCascadeSystem; // [추가됨]
	USoundBase* SoundToPlay = DefaultLoopSound;

	// 2. Payload(DataAsset)에서 오버라이드 값 추출
	if (Parameters.EffectContext.IsValid())
	{
		if (const USFDA_BuffAuraEffectData* EffectData = Cast<USFDA_BuffAuraEffectData>(Parameters.EffectContext.GetSourceObject()))
		{
			if (EffectData->NiagaraSystem)
			{
				NiagaraToSpawn = EffectData->NiagaraSystem;
			}
			if (EffectData->LoopSound)
			{
				SoundToPlay = EffectData->LoopSound;
			}
			
			// [참고] 만약 USFDA_BuffAuraEffectData 안에 캐스케이드 변수(예: ParticleSystem)를 추가했다면
			// 아래와 같이 연결해주시면 됩니다. 현재는 헤더 내용을 몰라 주석 처리합니다.
			/*
			if (EffectData->CascadeSystem) 
			{
				CascadeToSpawn = EffectData->CascadeSystem;
			}
			*/
		}
	}

	// 3. 부착 컴포넌트 결정
	USceneComponent* AttachComponent = nullptr;
	if (ACharacter* Character = Cast<ACharacter>(MyTarget))
	{
		AttachComponent = Character->GetMesh();
	}
	else
	{
		AttachComponent = MyTarget->GetRootComponent();
	}

	if (!AttachComponent)
	{
		return false;
	}

	// 4. 나이아가라 FX 부착
	if (NiagaraToSpawn)
	{
		SpawnedNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraToSpawn,
			AttachComponent,
			AttachSocketName,
			LocationOffset,
			RotationOffset,
			FXScale,
			EAttachLocation::KeepRelativeOffset,
			true,
			ENCPoolMethod::None,
			true
		);
	}

	// 5. [추가됨] 캐스케이드 FX 부착
	if (CascadeToSpawn)
	{
		SpawnedCascadeComponent = UGameplayStatics::SpawnEmitterAttached(
			CascadeToSpawn,
			AttachComponent,
			AttachSocketName,
			LocationOffset,
			RotationOffset,
			FXScale,
			EAttachLocation::KeepRelativeOffset,
			true,
			EPSCPoolMethod::None,
			true
		);
	}

	// 6. 루프 사운드 부착
	if (SoundToPlay)
	{
		APawn* TargetPawn = Cast<APawn>(MyTarget);
		if (TargetPawn && TargetPawn->IsLocallyControlled())
		{
			SpawnedAudioComponent = UGameplayStatics::SpawnSoundAttached(
				SoundToPlay,
				AttachComponent,
				AttachSocketName
			);

			if (SpawnedAudioComponent)
			{
				SpawnedAudioComponent->bAutoDestroy = false;
			}
		}
	}

	return true;
}

bool ASFGCN_BuffAura::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	return true;
}

bool ASFGCN_BuffAura::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	// 나이아가라 정리
	if (SpawnedNiagaraComponent)
	{
		SpawnedNiagaraComponent->Deactivate();
		SpawnedNiagaraComponent->DestroyComponent();
		SpawnedNiagaraComponent = nullptr;
	}

	// [추가됨] 캐스케이드 정리
	if (SpawnedCascadeComponent)
	{
		SpawnedCascadeComponent->DeactivateSystem();
		SpawnedCascadeComponent->DestroyComponent();
		SpawnedCascadeComponent = nullptr;
	}

	// 사운드 정리
	if (SpawnedAudioComponent)
	{
		SpawnedAudioComponent->Stop();
		SpawnedAudioComponent->DestroyComponent();
		SpawnedAudioComponent = nullptr;
	}

	return true;
}