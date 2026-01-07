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

	// 번개 초기화 (부모의 InitAOE 대신 사용)
	void InitLightning(
		UAbilitySystemComponent* InSourceASC,
		AActor* InSourceActor,
		float InBaseDamage,
		float InBoltRadius,      // 번개 두께
		float InBoltHeight       // 번개 높이 (원기둥 구현용)
	);

protected:
	virtual void BeginPlay() override;

	// 부모의 Tick 데미지 로직 등을 무력화하기 위해 오버라이드
	virtual void OnDamageTick_Implementation(); 

	virtual void ApplyDamageToTargets(float DamageAmount, float EffectRadius) override;
	
protected:
	// 원기둥 형태 충돌을 위한 캡슐 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SF|Components")
	TObjectPtr<UCapsuleComponent> LightningCollision;
};