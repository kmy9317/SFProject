#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "Interface/SFEnemyAbilityInterface.h"
#include "SFGA_Enemy_BaseAttack.generated.h"

enum class EAIRotationMode : uint8;

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

UCLASS(Abstract)
class SF_API USFGA_Enemy_BaseAttack : public USFGameplayAbility, public ISFEnemyAbilityInterface
{
    GENERATED_BODY()

public:
    USFGA_Enemy_BaseAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
    
    //~ Begin UGameplayAbility Interface
    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        OUT FGameplayTagContainer* OptionalRelevantTags = nullptr
    ) const override;
    
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData
    ) override;
    
    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled
    ) override;
    //~ End UGameplayAbility Interface
    
    //~ Begin ISFEnemyAbilityInterface
    virtual float CalcAIScore(const FEnemyAbilitySelectContext& Context) const override;
    virtual float CalcScoreModifier(const FEnemyAbilitySelectContext& Context) const override;
    //~ End ISFEnemyAbilityInterface
    
    // 공격 가능 여부 체크
    virtual bool CanAttackTarget(const FEnemyAbilitySelectContext& Context) const;
    
    // 공격 가능 거리 체크 (Actor 버전)
    UFUNCTION(BlueprintCallable, Category = "Attack")
    virtual bool IsWithinAttackRange(const AActor* Target) const;
    virtual bool IsWithinAttackRange(const FEnemyAbilitySelectContext& Context) const;

    // 공격 가능 각도 체크
    UFUNCTION(BlueprintCallable, Category = "Attack")
    virtual bool IsWithinAttackAngle(const AActor* Target) const;
    virtual bool IsWithinAttackAngle(const FEnemyAbilitySelectContext& Context) const;
  
    // Getter Functions
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
    virtual float GetAttackAngle() const;

    // 넉백/런치 적용
    virtual void ApplyKnockBackToTarget(AActor* Target, const FVector& HitLocation) const;
    virtual void ApplyLaunchToTarget(AActor* Target, const FVector& LaunchDirection, float Magnitude = 1.0f) const;

protected:
    // 사거리/각도 체크 헬퍼 
    bool CheckRangeAndAngle(const AActor* Self, const AActor* Target, const FGameplayAbilitySpec* Spec) const;
 
    // 데미지 적용 
    UFUNCTION(BlueprintCallable, Category = "Attack")
    void ApplyDamageToTarget(AActor* Target, const FGameplayEffectContextHandle& ContextHandle);

    // Raw 데미지 적용
    UFUNCTION(BlueprintCallable, Category = "Attack")
    void ApplyRawDamageToTarget(AActor* Target, float RawDamage, const FGameplayEffectContextHandle& ContextHandle);

    UFUNCTION(BlueprintCallable, Category = "Attack")
    virtual AActor* GetCurrentTarget() const;


    virtual void ApplyCooldown(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo
    ) const override;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Animation Montage")
    FTaggedMontage AttackTypeMontage;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    EAttackType AttackType;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    FGameplayTag CoolDownTag;

    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Damage")
    TSubclassOf<UGameplayEffect> DamageGameplayEffectClass;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Damage")
    TSubclassOf<UGameplayEffect> RawDamageGameplayEffectClass;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Cooldown")
    bool bHasCooldown = true;

private:
    float GetSetByCallerValue(const FGameplayTag& Tag, float DefaultValue = 0.0f) const;
};