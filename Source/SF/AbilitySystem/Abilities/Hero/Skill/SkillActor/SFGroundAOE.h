#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "SFGroundAOE.generated.h"

class USphereComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UAbilitySystemComponent;
class UGameplayEffect;

/**
 * 지면에 설치되어 주기적으로 범위 피해를 입히는 장판 액터
 * SFAttackProjectile의 데미지 처리 방식을 참고하여 제작됨
 */
UCLASS()
class SF_API ASFGroundAOE : public AActor
{
	GENERATED_BODY()
	
public:	
	ASFGroundAOE(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 어빌리티에서 생성 직후 호출하여 데이터 주입
	void InitAOE(
		UAbilitySystemComponent* InSourceASC,
		AActor* InSourceActor,
		float InBaseDamage,
		float InRadius,
		float InDuration,
		float InTickInterval
	);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 주기적 데미지 처리 함수
	UFUNCTION()
	void OnDamageTick();

	// 서버에서 실제 데미지 적용
	void ApplyDamageEffect_Server();

	// 지속 시간 종료 시
	void OnDurationExpired();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SF|Components")
	TObjectPtr<USphereComponent> AreaCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SF|Components")
	TObjectPtr<UNiagaraComponent> AreaEffect;

protected:
	// === 설정 가능 요소 === //
	
	// 장판 생성 시 재생할 사운드 (옵션)
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|FX")
	TObjectPtr<USoundBase> SpawnSound;

	// 매 타격 시 재생할 사운드 (옵션)
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|FX")
	TObjectPtr<USoundBase> TickSound;

	// 데미지와 별도로 적용할 디버프 GE (예: 슬로우, 방어력 감소)
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Effect")
	TSubclassOf<UGameplayEffect> DebuffGameplayEffectClass;
	
	// 데미지 적용에 사용할 기본 GE (SetByCaller 사용)
	// 비워둘 경우 SFAttackProjectile 처럼 GameData의 공용 GE를 사용하도록 구현 가능
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Effect")
	TSubclassOf<UGameplayEffect> DamageGameplayEffectClass;

	// SetByCaller로 데미지를 넣을 태그
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Effect")
	FGameplayTag SetByCallerDamageTag;

private:
	// 런타임 데이터
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	UPROPERTY()
	TWeakObjectPtr<AActor> SourceActor;

	float BaseDamage = 0.f;
	float AttackRadius = 300.f;
	
	FTimerHandle DurationTimerHandle;
	FTimerHandle TickTimerHandle;
};