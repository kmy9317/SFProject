// Fill out your copyright notice in the Description page of Project Settings.

#include "SFBTS_FindAttackPoint.h"
#include "AI/Controller/SFEnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"
#include "GameplayTagContainer.h"
#include "NavigationSystem.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/SFCharacterBase.h"

USFBTS_FindAttackPoint::USFBTS_FindAttackPoint()
{
    NodeName = "Find Attack Point By EQS";
    Interval = 1.0f;
    RandomDeviation = 0.2f;
}

void USFBTS_FindAttackPoint::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp) return;

    FName SelectedAbilityTagName = BlackboardComp->GetValueAsName(FName("SelectedAbilityTag"));
    AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKeyName));

    if (SelectedAbilityTagName == NAME_None || !TargetActor) return;

    FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(SelectedAbilityTagName);
    if (!AbilityTag.IsValid()) return;

    ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
    if (!AIController) return;

    float MinDist = 0;
    float MaxDist = 1000;
    USFGA_Enemy_BaseAttack* BaseAttack = nullptr;

    ASFCharacterBase* Character = Cast<ASFCharacterBase>(AIController->GetPawn());
    if (Character)
    {
        USFAbilitySystemComponent* ASC = Character->GetSFAbilitySystemComponent();
        if (ASC)
        {
            for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
            {
                if (Spec.Ability && Spec.Ability->AbilityTags.HasTag(AbilityTag))
                {
                    BaseAttack = Cast<USFGA_Enemy_BaseAttack>(Spec.GetPrimaryInstance());
                    if (BaseAttack)
                    {
                        MinDist = BaseAttack->GetMinAttackRange();
                        MaxDist = BaseAttack->GetAttackRange();
                        break;
                    }
                }
            }
        }
    }

    // [삭제] 이 부분이 있으면 몹이 뭉쳤을 때 뒤에 있는 몹이 "사거리는 되네?" 하고 멈춰버립니다.
    // 계속 움직여서 빈 공간을 찾도록 이 체크를 제거합니다.
    /*
    if (BaseAttack && TargetActor)
    {
        if (BaseAttack->IsWithinAttackRange(TargetActor))
        {
            return;
        }
    }
    */

    FBTRunEQSQueryMemory* MyMemory = reinterpret_cast<FBTRunEQSQueryMemory*>(NodeMemory);

    bool bShouldRunQuery = false;
    if (bDetectTargetChange && TargetActor != MyMemory->LastTargetActor.Get())
    {
        bShouldRunQuery = true;
        MyMemory->LastTargetActor = TargetActor;
    }
    if (AbilityTag != MyMemory->LastAbilityTag)
    {
        bShouldRunQuery = true;
        MyMemory->LastAbilityTag = AbilityTag;
    }

    if (MyMemory->QueryID != INDEX_NONE)
    {
        if (bShouldRunQuery)
        {
            UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(OwnerComp.GetWorld());
            if (QueryManager)
            {
                QueryManager->AbortQuery(MyMemory->QueryID);
                MyMemory->QueryID = INDEX_NONE;
            }
        }
        else
        {
            return;
        }
    }

    if (!QueryTemplate) return;

    UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(AIController->GetWorld());
    if (!QueryManager) return;

    FEnvQueryRequest QueryRequest(QueryTemplate, AIController);
    QueryRequest.SetFloatParam(FName("MinDistance"), MinDist);
    QueryRequest.SetFloatParam(FName("MaxDistance"), MaxDist);

    MyMemory->QueryID = QueryRequest.Execute(
        RunMode,
        FQueryFinishedSignature::CreateUObject(this, &USFBTS_FindAttackPoint::OnQueryFinished, &OwnerComp)
    );
}

void USFBTS_FindAttackPoint::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result, UBehaviorTreeComponent* OwnerComp)
{
    if (!OwnerComp) return;
    if (!Result.IsValid()) return;

    UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
    if (!BlackboardComp) return;

    FBTRunEQSQueryMemory* MyMemory = CastInstanceNodeMemory<FBTRunEQSQueryMemory>(
        OwnerComp->GetNodeMemory(this, OwnerComp->FindInstanceContainingNode(this))
    );

    if (Result->IsSuccessful())
    {
        FVector ResultLocation = Result->GetItemAsLocation(0);
        AAIController* AIController = OwnerComp->GetAIOwner();
        
        if (AIController)
        {
            UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(AIController->GetWorld());
            if (NavSys)
            {
                FNavLocation NavLoc;
                // [수정] 투영 박스를 넉넉하게 잡아서 미세한 오차 방지
                if (NavSys->ProjectPointToNavigation(ResultLocation, NavLoc, FVector(200.f, 200.f, 200.f)))
                {
                    ResultLocation = NavLoc.Location;
                    BlackboardComp->SetValueAsVector(ResultKeyName, ResultLocation);
                }
            }
            else
            {
                BlackboardComp->SetValueAsVector(ResultKeyName, ResultLocation);
            }
        }
    }

    MyMemory->QueryID = INDEX_NONE;
}

uint16 USFBTS_FindAttackPoint::GetInstanceMemorySize() const
{
    return sizeof(FBTRunEQSQueryMemory);
}

void USFBTS_FindAttackPoint::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
    Super::InitializeMemory(OwnerComp, NodeMemory, InitType);
    FBTRunEQSQueryMemory* MyMemory = reinterpret_cast<FBTRunEQSQueryMemory*>(NodeMemory);
    new (MyMemory) FBTRunEQSQueryMemory();
}