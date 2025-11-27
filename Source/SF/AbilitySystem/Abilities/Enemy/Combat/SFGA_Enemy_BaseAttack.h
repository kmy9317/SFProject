#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "Interface/SFEnemyAbilityInterface.h"
#include "SFGA_Enemy_BaseAttack.generated.h"

UENUM(BlueprintType)
enum class EAttackType : uint8
{
    Melee,
    Range
};

USTRUCT(BlueprintType)
struct FTaggedMontage
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Animation Montage")
    FGameplayTag Tag;

    UPROPERTY(EditDefaultsOnly, Category = "Animation Montage")
    TObjectPtr<UAnimMontage> AnimMontage;
};

/**
 * 
 */
UCLASS(Abstract)
class SF_API USFGA_Enemy_BaseAttack : public USFGameplayAbility, public ISFEnemyAbilityInterface
{
    GENERATED_BODY()

    

public:
    USFGA_Enemy_BaseAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
    
    // 공격 가능 거리 체크
    UFUNCTION(BlueprintCallable, Category = "Attack")
    virtual bool IsWithinAttackRange(const AActor* Target) const;

    // 공격 가능 각도 체크
    UFUNCTION(BlueprintCallable, Category = "Attack")
    virtual bool IsWithinAttackAngle(const AActor* Target) const;

    // 공격 가능 여부 종합 체크
    UFUNCTION(BlueprintCallable, Category = "Attack")
    virtual bool CanAttackTarget(const AActor* Target) const;

    // Getter 함수들
    UFUNCTION(BlueprintPure, Category = "Attack")
    EAttackType GetAttackType() const { return AttackType; }

    UFUNCTION(BlueprintPure, Category = "Attack")
    float GetAttackRange() const { return AttackRange; }

    UFUNCTION(BlueprintPure, Category = "Attack")
    float GetMinAttackRange() const { return MinAttackRange; }

    UFUNCTION(BlueprintPure, Category = "Attack")
    float GetBaseDamage() const { return BaseDamage; }

    virtual float CalcAIScore(const FEnemyAbilitySelectContext& Context) const override;
    

    
protected:
 
    UFUNCTION(BlueprintCallable, Category = "Attack")
    virtual void ApplyDamageToTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "Attack")
    virtual ASFCharacterBase* GetCurrentTarget() const;

    virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
protected:
    UPROPERTY(EditDefaultsOnly, Category = "Animation Montage")
    FTaggedMontage AttackTypeMontage;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    EAttackType AttackType;

    // 공격 범위
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    float AttackRange = 200.0f;

    // 최소 공격 거리 
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    float MinAttackRange = 0.0f;

    // 공격 데미지
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    float BaseDamage = 10.0f;

    // 공격 쿨다운
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    float Cooldown = 1.0f;

    // 공격 각도
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    float AttackAngle = 45.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    FGameplayTag CoolDownTag;

    // 데미지 GameplayEffect 클래스
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Damage")
    TSubclassOf<UGameplayEffect> DamageGameplayEffectClass;
    
};