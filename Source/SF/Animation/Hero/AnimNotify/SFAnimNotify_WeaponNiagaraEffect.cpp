#include "SFAnimNotify_WeaponNiagaraEffect.h"

#include "Character/SFCharacterBase.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "Weapons/Actor/SFEquipmentBase.h"

USFAnimNotify_WeaponNiagaraEffect::USFAnimNotify_WeaponNiagaraEffect()
{
#if WITH_EDITORONLY_DATA
	bShouldFireInEditor = false;
#endif
}

void USFAnimNotify_WeaponNiagaraEffect::Notify(USkeletalMeshComponent* MeshComponent, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	USkeletalMeshComponent* WeaponMeshComponent = GetWeaponMeshComponent(MeshComponent);
	Super::Notify(WeaponMeshComponent, Animation, EventReference);
}

USkeletalMeshComponent* USFAnimNotify_WeaponNiagaraEffect::GetWeaponMeshComponent(USkeletalMeshComponent* CharacterMeshComponent) const
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
