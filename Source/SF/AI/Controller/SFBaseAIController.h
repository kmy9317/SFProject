#pragma once

#include "CoreMinimal.h"
#include "DetourCrowdAIController.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "Character/Enemy/SFEnemyData.h"
#include "Interface/SFAIControllerInterface.h"
#include "SFBaseAIController.generated.h"

class USFCombatComponentBase;
class UBehaviorTreeComponent;
class UBlackboardComponent;
class UBehaviorTree;

UENUM(BlueprintType)
enum class EAIRotationMode : uint8
{
    None,              // 회전 없음
    MovementDirection, // 이동 방향으로 회전 (비전투)
    ControllerYaw      // Controller 방향으로 회전 (전투)
};

UCLASS(Abstract)
class SF_API ASFBaseAIController : public ADetourCrowdAIController, public ISFAIControllerInterface
{
    GENERATED_BODY()

public:
    ASFBaseAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    //~ Begin ISFAIControllerInterface Interface
    virtual void InitializeAIController() override;
    virtual USFCombatComponentBase* GetCombatComponent() const override;
    //~ End ISFAIControllerInterface Interface

    //~ Begin IGenericTeamAgentInterface Interface
    virtual void SetGenericTeamId(const FGenericTeamId& InTeamID) override;
    virtual FGenericTeamId GetGenericTeamId() const override;
    //~ End IGenericTeamAgentInterface Interface

protected:
    //~ Begin AActor Interface
    virtual void PreInitializeComponents() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    //~ End AActor Interface

    //~ Begin AAIController Interface
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual bool RunBehaviorTree(UBehaviorTree* BehaviorTree) override;
    virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;
    //~ End AAIController Interface

#pragma region BehaviorTree
protected:
    // BehaviorTree 관리
    void ChangeBehaviorTree(FGameplayTag GameplayTag);
    void StopBehaviorTree();
    void SetBehaviorTree(UBehaviorTree* BehaviorTree);
    void BindingStateMachine(const APawn* InPawn);
    void UnBindingStateMachine();

    UPROPERTY()
    FSFBehaviourWrapperContainer BehaviorTreeContainer;

    UPROPERTY()
    TObjectPtr<UBehaviorTreeComponent> CachedBehaviorTreeComponent;

    UPROPERTY()
    TObjectPtr<UBlackboardComponent> CachedBlackboardComponent;
#pragma endregion

#pragma region StateReaction
protected:
    // 상태 반응 콜백
    UFUNCTION()
    void ReceiveStateStart(FGameplayTag StateTag);

    UFUNCTION()
    void ReceiveStateEnd(FGameplayTag StateTag);
#pragma endregion

#pragma region Combat
public:
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="AI|Combat")
    bool bIsInCombat = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Combat")
    TObjectPtr<AActor> TargetActor;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI|Combat")
    TObjectPtr<USFCombatComponentBase> CombatComponent;
#pragma endregion

#pragma region Team
protected:
    UPROPERTY(Replicated)
    FGenericTeamId TeamId;
#pragma endregion

#pragma region Rotation
public:
    
    void SetRotationMode(EAIRotationMode NewMode);
    EAIRotationMode GetCurrentRotationMode() const { return CurrentRotationMode; }

    
    virtual bool ShouldRotateActorByController() const;
    virtual bool IsTurningInPlace() const { return false; }
    virtual float GetTurnThreshold() const { return 45.0f; }

protected:
    UPROPERTY()
    EAIRotationMode CurrentRotationMode = EAIRotationMode::MovementDirection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Rotation", meta=(ClampMin="0.0", ClampMax="1080.0"))
    float RotationInterpSpeed = 5.f;
#pragma endregion
};