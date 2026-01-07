#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "SFAttackProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class UGameplayEffect;
class UAbilitySystemComponent;

UCLASS()
class SF_API ASFAttackProjectile : public AActor
{
	GENERATED_BODY()

public:
	ASFAttackProjectile(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// GA에서 발사 직후 호출: 소스 정보/데미지 주입
	void InitProjectile(UAbilitySystemComponent* InSourceASC, float InDamage, AActor* InSourceActor);
	// 차징 스킬용 초기화 함수 추가 (Scale, 폭발 여부 포함)
	void InitProjectileCharged(UAbilitySystemComponent* InSourceASC, float InDamage, AActor* InSourceActor, float InScale, bool bInExplodes);
	// 실제 발사(속도/방향 적용)
	void Launch(const FVector& Direction);

protected:
	virtual void BeginPlay() override;

protected:
	UFUNCTION()
	void OnProjectileHit(
		UPrimitiveComponent* HitComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);

	// ✅ Pawn 판정은 Overlap에서 처리
	UFUNCTION()
	void OnProjectileOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	// 서버에서 데미지/GE 적용
	void ApplyHitEffects_Server(AActor* TargetActor, const FHitResult& Hit);

	// 임팩트 FX/SFX (멀티캐스트로 클라이언트에도 표시)
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayImpactFX(const FVector& Location, const FVector& Normal);

	// 추가 폭발 공격
	void ProcessExplosion_Server(const FVector& Location);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SF|Projectile")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SF|Projectile")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SF|Projectile")
	TObjectPtr<UNiagaraComponent> TrailNiagara;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SF|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

protected:
	// === 설정 가능 요소들 (요구사항 4) ===

	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|FX")
	TObjectPtr<UNiagaraSystem> ImpactNiagara;

	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|FX")
	TObjectPtr<USoundBase> SpawnSound;

	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|FX")
	TObjectPtr<USoundBase> ImpactSound;

	// 데미지 적용에 사용할 GE (기본은 GameData의 SetByCaller Damage GE 사용 가능)
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|Effect")
	TSubclassOf<UGameplayEffect> DamageGameplayEffectClass;

	// 히트 시 추가로 부여할 GE (슬로우/화상 등)
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|Effect")
	TSubclassOf<UGameplayEffect> AdditionalHitGameplayEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|Effect")
	float AdditionalEffectLevel = 1.0f;

	// SetByCaller로 데미지를 넣을 태그 (프로젝트 태그에 맞춰 BP에서 변경 가능)
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|Effect")
	FGameplayTag SetByCallerDamageTag;

	// 생존 시간
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile")
	float LifeSeconds = 5.0f;

	// 발사 속도
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile")
	float InitialSpeed = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile")
	float MaxSpeed = 2000.f;

	// 맞으면 즉시 파괴
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile")
	bool bDestroyOnHit = true;

	// 추가 폭발 설정
	// 폭발 시 적용할 GE (범위 데미지 등)
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|Explosion")
	TSubclassOf<UGameplayEffect> ExplosionGameplayEffectClass;

	// 폭발 반경
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|Explosion")
	float ExplosionRadius = 300.f;

	// 디버그 드로잉 여부
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|Explosion")
	bool bDebugExplosion = false;

	// 폭발 GC
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|Explosion")
	FGameplayTag ExplosionCueTag;

	// 관통 여부 (True면 적/아군 충돌 시 파괴되지 않고 뚫고 지나감)
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile")
	bool bCanPierce = false;

	// 아군에게 버프 적용 여부 (True면 아군을 무시하지 않고 피격 판정)
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|Buff")
	bool bApplyBuffToFriendly = false;

	// 아군 피격 시 적용할 버프 GE
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|Buff")
	TSubclassOf<UGameplayEffect> BuffGameplayEffectClass;

	// 아군 버프 적용 처리를 위한 서버 함수
	void ApplyBuff_Server(AActor* TargetActor, const FHitResult& Hit);
	
private:
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	UPROPERTY()
	TWeakObjectPtr<AActor> SourceActor;

	UPROPERTY()
	float Damage = 0.f;
	
	UPROPERTY()
	bool bIsExplosive = false;
	
private:
	TSubclassOf<UGameplayEffect> ResolveDamageGE() const;
};
