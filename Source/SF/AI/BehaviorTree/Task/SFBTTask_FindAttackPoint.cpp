#include "SFBTTask_FindAttackPoint.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "NavigationSystem.h"
#include "VisualLogger/VisualLogger.h"

// Game Specific Headers
#include "Character/SFCharacterBase.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"

USFBTTask_FindAttackPoint::USFBTTask_FindAttackPoint(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    NodeName = "Find Attack Point (EQS)";
    
    // 필터 설정 (에디터 편의성)
    ResultKeyName.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(USFBTTask_FindAttackPoint, ResultKeyName));
    TargetActor.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(USFBTTask_FindAttackPoint, TargetActor), AActor::StaticClass());
    AbilityTagKeyName.AddNameFilter(this, GET_MEMBER_NAME_CHECKED(USFBTTask_FindAttackPoint, AbilityTagKeyName));
}

EBTNodeResult::Type USFBTTask_FindAttackPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    ASFCharacterBase* Character = AIController ? Cast<ASFCharacterBase>(AIController->GetPawn()) : nullptr;
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

    if (!Character || !Blackboard || !QueryTemplate)
    {
        return EBTNodeResult::Failed;
    }

    CachedOwnerComp = &OwnerComp;
    
    AActor* TargetActorPtr = Cast<AActor>(Blackboard->GetValueAsObject(TargetActor.SelectedKeyName));
    FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(Blackboard->GetValueAsName(AbilityTagKeyName.SelectedKeyName)); 

    if (!TargetActorPtr || !AbilityTag.IsValid())
    {
        return EBTNodeResult::Failed;
    }

     float MinDist = 0.f;
    float MaxDist = 1000.f; 
    bool bFoundAbility = false;

    USFAbilitySystemComponent* ASC = Character->GetSFAbilitySystemComponent();
    if (ASC)
    {
        for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
        {
            // 태그 매칭 확인 (HasTagExact 권장)
            if (Spec.Ability && Spec.Ability->AbilityTags.HasTag(AbilityTag))
            {
                // 1. Spec에 저장된 실시간 값 먼저 확인
                const float* MinValPtr = Spec.SetByCallerTagMagnitudes.Find(SFGameplayTags::Data_EnemyAbility_MinAttackRange);
                const float* MaxValPtr = Spec.SetByCallerTagMagnitudes.Find(SFGameplayTags::Data_EnemyAbility_AttackRange);

                if (MinValPtr && MaxValPtr)
                {
                    MinDist = *MinValPtr;
                    MaxDist = *MaxValPtr;
                }
                else
                {
                    
                    if (USFGA_Enemy_BaseAttack* CDO = Cast<USFGA_Enemy_BaseAttack>(Spec.Ability))
                    {
                        MinDist = CDO->GetMinAttackRange();
                        MaxDist = CDO->GetAttackRange();
                    }
                }
            
                bFoundAbility = true;
                break;
            }
        }
    }

    

    if (bSkipIfInRange && TargetActorPtr)
    {
        // 수평 거리(2D)로 체크하여 높이 오차 제거
        float DistSq = FVector::DistSquared2D(Character->GetActorLocation(), TargetActorPtr->GetActorLocation());
        float MaxRangeSq = MaxDist * MaxDist;
        
        // 90% 사거리 안에 있다면 이미 충분함
        if (DistSq < (MaxRangeSq * 0.81f)) // 0.9의 제곱은 0.81
        {
            return EBTNodeResult::Succeeded;
        }
    }

    // 4. EQS 파라미터 주입
    FEnvQueryRequest QueryRequest(QueryTemplate, Character);
    const float OptimalDistance = (MinDist + MaxDist) * 0.5f;

    QueryRequest.SetFloatParam(FName("MinDistance"), MinDist);
    QueryRequest.SetFloatParam(FName("MaxDistance"), MaxDist);
    QueryRequest.SetFloatParam(FName("OptimalDistance"), OptimalDistance);


    // 쿼리 실행
    QueryID = QueryRequest.Execute(RunMode, this, &USFBTTask_FindAttackPoint::OnQueryFinished);

    if (QueryID == INDEX_NONE)
    {
        return EBTNodeResult::Failed;
    }

    return EBTNodeResult::InProgress;
}

void USFBTTask_FindAttackPoint::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
    if (!CachedOwnerComp.IsValid() || QueryID == INDEX_NONE)
    {
        return;
    }

    // 현재 실행 중인 쿼리 결과인지 확인
    if (Result->QueryID != QueryID)
    {
        return;
    }

    QueryID = INDEX_NONE;
    
    if (Result->IsAborted())
    {
        FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Aborted);
        return;
    }

    if (Result->IsSuccessful())
    {
        FVector ResultLocation = Result->GetItemAsLocation(0);
        UBlackboardComponent* Blackboard = CachedOwnerComp->GetBlackboardComponent();

        // NavMesh 투영 (참고 코드의 로직 반영)
        if (bProjectToNavMesh)
        {
            const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
            if (NavSys)
            {
                FNavLocation ProjectedLocation;
                if (NavSys->ProjectPointToNavigation(ResultLocation, ProjectedLocation, FVector(200.f, 200.f, 200.f)))
                {
                    ResultLocation = ProjectedLocation.Location;
                }
            }
        }

        // 결과 블랙보드에 쓰기
        if (Blackboard)
        {
            Blackboard->SetValueAsVector(ResultKeyName.SelectedKeyName, ResultLocation);
        }

        FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
    }
    else
    {
        // 쿼리 실패 시 지정된 실패 결과 반환
        FinishLatentTask(*CachedOwnerComp, FailureResult);
    }
}

EBTNodeResult::Type USFBTTask_FindAttackPoint::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (QueryID != INDEX_NONE)
    {
        UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(GetWorld());
        if (QueryManager)
        {
            QueryManager->AbortQuery(QueryID);
        }
        QueryID = INDEX_NONE;
    }

    return Super::AbortTask(OwnerComp, NodeMemory);
}

void USFBTTask_FindAttackPoint::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    CachedOwnerComp.Reset();
    QueryID = INDEX_NONE;

    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}