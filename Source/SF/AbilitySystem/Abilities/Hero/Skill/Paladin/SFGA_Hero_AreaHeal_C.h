#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Skill_Melee.h"
#include "SFGA_Hero_AreaHeal_C.generated.h"

class UParticleSystem;
class USoundBase;
class UAnimMontage;
class UGameplayEffect;
class UNiagaraSystem;
class UNiagaraComponent;
class USkeletalMeshComponent;

UCLASS()
class SF_API USFGA_Hero_AreaHeal_C : public USFGA_Skill_Melee
{
	GENERATED_BODY()

public:
	USFGA_Hero_AreaHeal_C(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	//Ability 실행
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;
	
	//Ability 종료
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

	//이펙트 & 타격 발생
	UFUNCTION()
	void OnLightningImpact(FGameplayEventData Payload);
	
	UPROPERTY(EditDefaultsOnly, Category="SF|GameplayEffect")
	TSubclassOf<UGameplayEffect> DamageGE;
	
	//몽타주 종료 시 실행
	UFUNCTION()
	void OnMontageEnded();

	//스킬 애님 몽타주
	UPROPERTY(EditDefaultsOnly, Category="SF|Animation")
	TObjectPtr<UAnimMontage> LightningMontage;

	//FX
	UPROPERTY(EditDefaultsOnly, Category="SF|Effect")
	UParticleSystem* LightningEffect1;
	UPROPERTY(EditDefaultsOnly, Category="SF|Effect")
	UParticleSystem* LightningEffect2;

	//Sound
	UPROPERTY(EditDefaultsOnly, Category="SF|Sound")
	USoundBase* LightningSound1;
	UPROPERTY(EditDefaultsOnly, Category="SF|Sound")
	USoundBase* LightningSound2;

	//GE
	UPROPERTY(EditDefaultsOnly, Category="SF|GameplayEffect")
	TSubclassOf<UGameplayEffect> DebuffGE;

	//스킬 설정
	UPROPERTY(EditDefaultsOnly, Category="SF|Damage")
	float StrikeDistance = 300.f;

	UPROPERTY(EditDefaultsOnly, Category="SF|Damage")
	float StrikeRadius = 200.f;

	//===================== Trail 관련 =====================
	UPROPERTY(EditDefaultsOnly, Category="SF|VFX")
	UNiagaraSystem* SwordTrailFX;

	UPROPERTY()
	UNiagaraComponent* TrailComp;

	FTimerHandle TrailUpdateHandle;
	FTimerHandle TrailFadeHandle;
	//=====================================================

	//현재 장착 무기 메쉬(예: BP_OneHandSword의 SKM_OneHandSword_001) 찾기
	USkeletalMeshComponent* FindCurrentWeaponMesh(ASFCharacterBase* OwnerChar) const;
};
