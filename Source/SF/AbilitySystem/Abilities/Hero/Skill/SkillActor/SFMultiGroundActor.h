#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SkillActor/SFGroundAOE.h"
#include "SFMultiGroundActor.generated.h"

class UCapsuleComponent;

/**
 * 번개 액터
 * - 원기둥(Capsule) 형태의 충돌체
 * - 소환 즉시 데미지를 입히고 시각 효과 후 사라짐
 */
UCLASS()
class SF_API ASFMultiGroundActor : public ASFGroundAOE
{
	GENERATED_BODY()
	
public:
	ASFMultiGroundActor(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ ISFPoolable Interface Override
	virtual void OnAcquiredFromPool() override;
	virtual void OnReturnedToPool() override;
	//~ End ISFPoolable Interface Override
	
	// 번개 초기화 (부모의 InitAOE 대신 사용)
	void InitLightning(UAbilitySystemComponent* InSourceASC,AActor* InSourceActor,float InBaseDamage,float InBoltRadius,float InBoltHeight, float InScale = 1.0f);

protected:
	virtual void BeginPlay() override;
	virtual void ApplyDamageToTargets(float DamageAmount, float EffectRadius) override;
	virtual void UpdateAOESize() override;

private:
	UFUNCTION()
	void OnRep_LightningScale();

private:
	void OnLifeTimeExpired(); 
	
protected:
	// 원기둥 형태 충돌을 위한 캡슐 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SF|Components")
	TObjectPtr<UCapsuleComponent> LightningCollision;

	UPROPERTY(ReplicatedUsing = OnRep_LightningScale)
	float LightningScale = 1.0f;

	FTimerHandle PoolReturnTimerHandle;
};