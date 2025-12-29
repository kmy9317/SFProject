#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ControllerComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "SFEnemyCombatComponent.generated.h"

class USFCombatSet_Enemy;
class USFAbilitySystemComponent;
struct FEnemyAbilitySelectContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatStateChanged, bool, bInCombat);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFEnemyCombatComponent : public UControllerComponent
{
    GENERATED_BODY()

public:
    USFEnemyCombatComponent(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintPure, Category = "SF|Combat")
    static USFEnemyCombatComponent* FindSFEnemyCombatComponent(const AController* Controller) 
    { 
        return (Controller ? Controller->FindComponentByClass<USFEnemyCombatComponent>() : nullptr); 
    }
    
    // ASC, AttributeSet 캐싱 및 Perception 콜백 바인딩
    virtual void InitializeCombatComponent();

    // Perception 이벤트: 단일 타겟 감지/상실 처리
    UFUNCTION()
    void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    // Perception 이벤트: 여러 타겟 동시 업데이트 처리
    UFUNCTION()
    void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors);

    // 현재 인지된 모든 타겟 중 베스트 타겟 선택
    void EvaluateBestTarget();

    // 타겟 변경 및 블랙보드/컨트롤러 동기화
    void UpdateTargetActor(AActor* NewTarget);

    // AI 상황에 맞는 최적 어빌리티 선택
    virtual bool SelectAbility(const FEnemyAbilitySelectContext& Context, const FGameplayTagContainer& SearchTags, FGameplayTag& OutSelectedTag);

    AActor* GetCurrentTarget() const { return CurrentTarget; }

    UPROPERTY(BlueprintAssignable, Category = "Combat")
    FOnCombatStateChanged OnCombatStateChanged;

protected:
    // 타겟의 우선순위 점수 계산 (거리, 시야 방향 기반)
    float CalculateTargetScore(AActor* Target) const;

    // 타겟과의 거리 기반으로 전투 범위 태그 업데이트
    void UpdateCombatRangeTags();

    // Attribute 변경 시 Perception 설정 동기화
    void UpdatePerceptionConfig();

    // GameplayTag 추가/제거
    void SetGameplayTagStatus(const FGameplayTag& Tag, bool bActive);

    // Owner Pawn 반환
    APawn* GetOwnerPawn() const;

    // 주기적 타겟 재평가 타이머 시작
    void StartTargetEvaluationTimer();

    // 타이머 중지 및 초기화
    void StopTargetEvaluationTimer();

    // 타이머 콜백: 타겟 재평가 + 거리 태그 업데이트
    UFUNCTION()
    void OnTargetEvaluationTimer();

protected:
    UPROPERTY()
    TObjectPtr<USFCombatSet_Enemy> CachedCombatSet;

    UPROPERTY()
    TObjectPtr<USFAbilitySystemComponent> CachedASC;
    
    UPROPERTY()
    TObjectPtr<AActor> CurrentTarget;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float ScoreDifferenceThreshold = 100.f;

    UPROPERTY(EditAnywhere, Category = "Combat|Target")
    float TargetEvaluationInterval = 0.5f;
    
    FTimerHandle TargetEvaluationTimerHandle;
};