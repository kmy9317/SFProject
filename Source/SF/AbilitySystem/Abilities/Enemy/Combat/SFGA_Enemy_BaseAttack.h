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


 // Enemy 기본 공격 Ability
 //DataTable의 값을 SetByCaller로 받아와서 사용
 
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

    
    UFUNCTION(BlueprintPure, Category = "Attack")
    EAttackType GetAttackType() const { return AttackType; }

    UFUNCTION(BlueprintPure, Category = "Attack")
    float GetAttackRange() const;

    UFUNCTION(BlueprintPure, Category = "Attack")
    float GetMinAttackRange() const;

    UFUNCTION(BlueprintPure, Category = "Attack")
    float GetBaseDamage() const;

    UFUNCTION(BlueprintPure, Category = "Attack")
    float GetCooldown() const;

    UFUNCTION(BlueprintPure, Category = "Attack")
    float GetAttackAngle() const;

    virtual float CalcAIScore(const FEnemyAbilitySelectContext& Context) const override;


protected:
    // 계산 있는 데미지 적용
    UFUNCTION(BlueprintCallable, Category = "Attack")
    void ApplyDamageToTarget(AActor* Target, const FGameplayEffectContextHandle& ContextHandle);

    // 계산 없는 순수 데미지 적용
    UFUNCTION(BlueprintCallable, Category = "Attack")
    void ApplyRawDamageToTarget(AActor* Target, float RawDamage, const FGameplayEffectContextHandle& ContextHandle);

    UFUNCTION(BlueprintCallable, Category = "Attack")
    virtual AActor* GetCurrentTarget() const;

    virtual void CommitExecute(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

    virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    
    UPROPERTY(EditDefaultsOnly, Category = "Animation Montage")
    FTaggedMontage AttackTypeMontage;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    EAttackType AttackType;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    FGameplayTag CoolDownTag;

    // 데미지 GameplayEffect 클래스 (Calculation 있음: 공격력, 방어력, 크리티컬 적용)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Damage")
    TSubclassOf<UGameplayEffect> DamageGameplayEffectClass;

    // Raw 데미지 GameplayEffect 클래스 (Calculation 없음: 순수 데미지)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Damage")
    TSubclassOf<UGameplayEffect> RawDamageGameplayEffectClass;

private:
    // SetByCaller 값 가져오기 헬퍼 함수
    float GetSetByCallerValue(const FGameplayTag& Tag, float DefaultValue = 0.0f) const;
};