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

UCLASS()
class SF_API ASFGroundAOE : public AActor
{
	GENERATED_BODY()
	
public:	
	ASFGroundAOE(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 초기화 함수
	void InitAOE(UAbilitySystemComponent* InSourceASC,AActor* InSourceActor,float InBaseDamage,float InRadius,float InDuration,float InTickInterval,float InExplosionRadius = -1.0f, float InExplosionDamageMultiplier = -1.0f,bool bOverrideExplodeOnEnd = false, bool bForceExplode = false );

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnDamageTick();

	virtual void ApplyDamageToTargets(float DamageAmount, float EffectRadius);
	void OnDurationExpired();
	void ExecuteRemovalGameplayCue();
	void ExecuteExplosion();

	UFUNCTION()
	void OnRep_AttackRadius();
	
	virtual void UpdateAOESize();
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SF|Components")
	TObjectPtr<USphereComponent> AreaCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SF|Components")
	TObjectPtr<UNiagaraComponent> AreaEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SF|Components")
	TObjectPtr<UParticleSystemComponent> AreaEffectCascade;

protected:
	// === [변경] 에디터에서 설정 가능한 폭발 옵션 === //
	
	// 폭발 범위 (기본값 300). 0보다 크면 Tick 범위와 다르게 적용됨.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SF|AOE|Explosion")
	float ExplosionRadius = 300.f;

	// 폭발 데미지 배율 (기본 1.0 = 100%).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SF|AOE|Explosion")
	float ExplosionDamageMultiplier = 1.0f;

	// 지속시간 종료 시 폭발 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SF|AOE|Explosion")
	bool bExplodeOnEnd = false;

	// === 기타 설정 === //
	
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|FX")
	TObjectPtr<USoundBase> SpawnSound;

	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|FX")
	TObjectPtr<USoundBase> TickSound;

	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|FX")
	FGameplayTag RemoveGameplayCueTag;

	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|FX")
	FGameplayTag ExplosionGameplayCueTag;
	
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Effect")
	TSubclassOf<UGameplayEffect> DebuffGameplayEffectClass;
	
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Effect")
	TSubclassOf<UGameplayEffect> DamageGameplayEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Effect")
	FGameplayTag SetByCallerDamageTag;
	
	// 런타임 데이터
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	UPROPERTY()
	TWeakObjectPtr<AActor> SourceActor;

	float BaseDamage = 0.f;
	
	UPROPERTY(ReplicatedUsing = OnRep_AttackRadius)
	float AttackRadius = 300.f;
	
	FTimerHandle DurationTimerHandle;
	FTimerHandle TickTimerHandle;
};