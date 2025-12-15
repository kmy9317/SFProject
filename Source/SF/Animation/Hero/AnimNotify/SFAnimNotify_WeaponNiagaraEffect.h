#pragma once

#include "CoreMinimal.h"
#include "AnimNotify_PlayNiagaraEffect.h"
#include "GameplayTagContainer.h"
#include "SFAnimNotify_WeaponNiagaraEffect.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class SF_API USFAnimNotify_WeaponNiagaraEffect : public UAnimNotify_PlayNiagaraEffect
{
	GENERATED_BODY()
public:
	USFAnimNotify_WeaponNiagaraEffect();

	virtual void Notify(USkeletalMeshComponent* MeshComponent, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	
private:
	USkeletalMeshComponent* GetWeaponMeshComponent(USkeletalMeshComponent* CharacterMeshComponent) const;

	UPROPERTY(EditAnywhere)
	FGameplayTag EquipmentSlotTag;
};
