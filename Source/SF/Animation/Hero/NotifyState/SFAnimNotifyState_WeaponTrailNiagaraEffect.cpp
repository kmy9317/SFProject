#include "SFAnimNotifyState_WeaponTrailNiagaraEffect.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/SFCharacterBase.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "Weapons/Actor/SFEquipmentBase.h"

USFAnimNotifyState_WeaponTrailNiagaraEffect::USFAnimNotifyState_WeaponTrailNiagaraEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	bShouldFireInEditor = false;
#endif

	Template = nullptr;
	LocationOffset.Set(0.0f, 0.0f, 0.0f);
	RotationOffset = FRotator(0.0f, 0.0f, 0.0f);
}

void USFAnimNotifyState_WeaponTrailNiagaraEffect::NotifyBegin(class USkeletalMeshComponent* MeshComponent,class UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	USkeletalMeshComponent* WeaponMeshComponent = GetWeaponMeshComponent(MeshComponent);
	Super::NotifyBegin(WeaponMeshComponent, Animation, TotalDuration, EventReference);

	UpdateNiagaraParameters(WeaponMeshComponent);
}

void USFAnimNotifyState_WeaponTrailNiagaraEffect::NotifyTick(USkeletalMeshComponent* MeshComponent,UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	USkeletalMeshComponent* WeaponMeshComponent = GetWeaponMeshComponent(MeshComponent);
	Super::NotifyTick(WeaponMeshComponent, Animation, FrameDeltaTime, EventReference);

	UpdateNiagaraParameters(WeaponMeshComponent);
}

void USFAnimNotifyState_WeaponTrailNiagaraEffect::NotifyEnd(class USkeletalMeshComponent* MeshComponent,class UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	USkeletalMeshComponent* WeaponMeshComponent = GetWeaponMeshComponent(MeshComponent);
	Super::NotifyEnd(WeaponMeshComponent, Animation, EventReference);

	UpdateNiagaraParameters(WeaponMeshComponent);
}

UFXSystemComponent* USFAnimNotifyState_WeaponTrailNiagaraEffect::SpawnEffect(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) const
{
	bool bValid = true;

	if (!Template)
	{
		bValid = false;
	}

	else if (!MeshComp->DoesSocketExist(SocketName) && MeshComp->GetBoneIndex(SocketName) == INDEX_NONE)
	{
		bValid = false;
	}
	
	if (bValid)
	{
		FFXSystemSpawnParameters SpawnParams;
		SpawnParams.SystemTemplate		= Template;
		SpawnParams.AttachToComponent	= MeshComp;
		SpawnParams.AttachPointName		= SocketName;
		SpawnParams.Location			= LocationOffset;
		SpawnParams.Rotation			= RotationOffset;
		SpawnParams.Scale				= Scale;
		SpawnParams.LocationType		= EAttachLocation::KeepRelativeOffset;
		SpawnParams.bAutoDestroy		= !bDestroyAtEnd;

		if (UNiagaraComponent* NewComponent = UNiagaraFunctionLibrary::SpawnSystemAttachedWithParams(SpawnParams))
		{
			if (bApplyRateScaleAsTimeDilation)
			{
				NewComponent->SetCustomTimeDilation(Animation->RateScale);
			}
			return NewComponent;
		}
	}
	return nullptr;
}

void USFAnimNotifyState_WeaponTrailNiagaraEffect::UpdateNiagaraParameters(USkeletalMeshComponent* WeaponMeshComponent)
{
	if (WeaponMeshComponent->DoesSocketExist(SocketName))
	{
		int32 Test = 1;
	}
	
	if (WeaponMeshComponent == nullptr)
	{
		return;
	}
	
	UNiagaraComponent* NiagaraComponent = Cast<UNiagaraComponent>(GetSpawnedEffect(WeaponMeshComponent));
	if (NiagaraComponent == nullptr)
	{
		return;
	}
	
	if (WeaponMeshComponent->DoesSocketExist(StartSocketName))
	{
		NiagaraComponent->SetVectorParameter(StartParameterName, WeaponMeshComponent->GetSocketLocation(StartSocketName));
	}

	if (WeaponMeshComponent->DoesSocketExist(EndSocketName))
	{
		NiagaraComponent->SetVectorParameter(EndParameterName, WeaponMeshComponent->GetSocketLocation(EndSocketName));
	}
}

USkeletalMeshComponent* USFAnimNotifyState_WeaponTrailNiagaraEffect::GetWeaponMeshComponent(USkeletalMeshComponent* CharacterMeshComponent) const
{
	USkeletalMeshComponent* WeaponMeshComponent = nullptr;

	if (ASFCharacterBase* Character = Cast<ASFCharacterBase>(CharacterMeshComponent->GetOwner()))
	{
		if (USFEquipmentComponent* EquipmentComponent = Character->FindComponentByClass<USFEquipmentComponent>())
		{
			if (ASFEquipmentBase* WeaponActor = Cast<ASFEquipmentBase>(EquipmentComponent->GetFirstEquippedActorBySlot(EquipmentSlotTag)))
			{
				WeaponMeshComponent = WeaponActor->GetMeshComponent();
			}
		}
	}
	return WeaponMeshComponent;
}
