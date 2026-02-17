#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/SFGA_Equipment_Base.h" // 부모 클래스 변경
#include "GameplayTagContainer.h"
#include "ScalableFloat.h" // BaseDamage용 구조체 포함
#include "SFGA_Hero_ProjectileLaunch.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class ASFAttackProjectile;
class UAnimMontage;

/**
 * 원거리 발사체 스킬
 */
UCLASS()
class SF_API USFGA_Hero_ProjectileLaunch : public USFGA_Equipment_Base
{
	GENERATED_BODY()

public:
	USFGA_Hero_ProjectileLaunch(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 레벨에 따른 기본 데미지 반환
	UFUNCTION(BlueprintCallable, Category = "SF|Damage")
	float GetScaledBaseDamage() const;
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,bool bReplicateCancelAbility) override;

	// 노티파이(게임플레이 이벤트) 수신 → 발사체 스폰
	UFUNCTION()
	virtual void OnProjectileSpawnEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

protected:
	// 스폰 위치(메인핸드 무기 소켓) 계산
	bool GetProjectileSpawnTransform(FTransform& OutSpawnTM) const;

	// 발사 방향(바라보던 방향) 계산
	FVector GetLaunchDirection() const;

	// 실제 발사체 스폰(서버)
	void SpawnProjectile_Server(const FTransform& SpawnTM, const FVector& LaunchDir);

protected:

	UPROPERTY(EditDefaultsOnly, Category="SF|Damage")
	FScalableFloat BaseDamage = 10.f;
	
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile")
	TSubclassOf<ASFAttackProjectile> ProjectileClass;

	// 몽타주
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|Montage")
	TObjectPtr<UAnimMontage> LaunchMontage;

	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|Montage")
	float LaunchMontagePlayRate = 1.0f;

	// 몽타주 노티파이에서 발생시키는 GameplayEventTag
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|Montage")
	FGameplayTag ProjectileSpawnEventTag;

	// 무기(메인핸드)에서 이 소켓 위치로 발사체를 생성
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|Socket")
	FName SpawnSocketName = TEXT("Muzzle");

	// 소켓이 없거나 무기 액터를 못 찾으면, 캐릭터 기준 오프셋으로 스폰
	UPROPERTY(EditDefaultsOnly, Category="SF|Projectile|Socket")
	FVector FallbackSpawnOffset = FVector(50.f, 0.f, 60.f);

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitEventTask;
};