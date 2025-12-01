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

    // ========================================
    // 1. 기본 검증
    // ========================================
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return;
    }

    // Ability Tag 가져오기
    FName SelectedAbilityTagName = BlackboardComp->GetValueAsName(
        FName("SelectedAbilityTag")
    );

    AActor* TargetActor = Cast<AActor>(
        BlackboardComp->GetValueAsObject(TargetActorKeyName)
    );

    if (SelectedAbilityTagName == NAME_None)
    {
        return;
    }

    if (!TargetActor)
    {
        return;
    }

    // FName → FGameplayTag 변환
    FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(SelectedAbilityTagName);

    if (!AbilityTag.IsValid())
    {
        return;
    }

    // ========================================
    // 2. AIController 가져오기
    // ========================================
    ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
    if (!AIController)
    {
        return;
    }

    // ========================================
    // 3. Ability 찾기 및 Range 값 가져오기
    // ========================================
    float MinDist = 0;
    float MaxDist = 1000;
    USFGA_Enemy_BaseAttack* BaseAttack = nullptr;

    ASFCharacterBase* Character = Cast<ASFCharacterBase>(AIController->GetPawn());
    if (Character)
    {
        USFAbilitySystemComponent* ASC = Character->GetSFAbilitySystemComponent();
        if (ASC)
        {
            // Ability 찾기
            for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
            {
                if (Spec.Ability)
                {
                    // Tag 매칭 확인
                    bool bTagMatches = false;

                    for (const FGameplayTag& SpecAbilityTag : Spec.Ability->AbilityTags)
                    {
                        if (SpecAbilityTag.MatchesTag(AbilityTag))
                        {
                            bTagMatches = true;
                            break;
                        }
                    }

                    if (!bTagMatches && Spec.Ability->AbilityTags.Num() == 0)
                    {
                        for (const FGameplayTag& AssetTag : Spec.Ability->GetAssetTags())
                        {
                            if (AssetTag.MatchesTag(AbilityTag))
                            {
                                bTagMatches = true;
                                break;
                            }
                        }
                    }

                    if (bTagMatches)
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
    }

    //범위 내면 수정 x 
    if (BaseAttack && TargetActor)
    {
        if (BaseAttack->IsWithinAttackRange(TargetActor))
        {
            // 이미 공격 범위 안에 있으면 EQS 실행 불필요
            return;
        }
    }

    // ========================================
    // 5. NodeMemory 가져오기
    // ========================================
    FBTRunEQSQueryMemory* MyMemory = reinterpret_cast<FBTRunEQSQueryMemory*>(NodeMemory);

    // ========================================
    // 6. 변경 감지
    // ========================================
    bool bShouldRunQuery = false;

    // 타겟 변경 감지
    if (bDetectTargetChange && TargetActor != MyMemory->LastTargetActor.Get())
    {
        bShouldRunQuery = true;
        MyMemory->LastTargetActor = TargetActor;
    }

    // Ability 변경 감지
    if (AbilityTag != MyMemory->LastAbilityTag)
    {
        bShouldRunQuery = true;
        MyMemory->LastAbilityTag = AbilityTag;
    }

    // ========================================
    // 7. 쿼리 실행 조건 체크
    // ========================================

    // 이전 쿼리가 실행 중인 경우
    if (MyMemory->QueryID != INDEX_NONE)
    {
        // 변경이 감지된 경우에만 중단 후 재실행
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
            // 변경 없고 쿼리 실행 중이면 대기
            return;
        }
    }

    // ========================================
    // 8. EQS Query 실행
    // ========================================
    if (!QueryTemplate)
    {
        return;
    }

    UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(AIController->GetWorld());
    if (!QueryManager) return;

    // EQS Query 요청 생성
    FEnvQueryRequest QueryRequest(QueryTemplate, AIController);

    QueryRequest.SetFloatParam(FName("MinDistance"), MinDist);
    QueryRequest.SetFloatParam(FName("MaxDistance"), MaxDist);

    // 쿼리 실행
    MyMemory->QueryID = QueryRequest.Execute(
        RunMode,
        FQueryFinishedSignature::CreateUObject(
            this,
            &USFBTS_FindAttackPoint::OnQueryFinished,
            &OwnerComp
        )
    );
}

void USFBTS_FindAttackPoint::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result, UBehaviorTreeComponent* OwnerComp)
{
    if (!OwnerComp) return;
    if (!Result.IsValid()) return;

    UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
    if (!BlackboardComp) return;

    // ✅ NodeMemory에서 QueryID 가져오기 
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

                const bool bOnNavMesh = NavSys->ProjectPointToNavigation(
                    ResultLocation,
                    NavLoc,
                    FVector(500.f, 500.f, 500.f)
                );

                if (bOnNavMesh)
                {
                    ResultLocation = NavLoc.Location;
                    BlackboardComp->SetValueAsVector(ResultKeyName, ResultLocation);
                }
            }
            else
            {
                // NavSys가 없으면 검증 없이 저장
                BlackboardComp->SetValueAsVector(ResultKeyName, ResultLocation);
            }
        }
    }

    //  핵심: 쿼리 완료 후 ID 초기화 
    MyMemory->QueryID = INDEX_NONE;
}

uint16 USFBTS_FindAttackPoint::GetInstanceMemorySize() const
{
    return sizeof(FBTRunEQSQueryMemory);
}


void USFBTS_FindAttackPoint::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
    EBTMemoryInit::Type InitType) const
{
    Super::InitializeMemory(OwnerComp, NodeMemory, InitType);
    
    FBTRunEQSQueryMemory* MyMemory = reinterpret_cast<FBTRunEQSQueryMemory*>(NodeMemory);
    new (MyMemory) FBTRunEQSQueryMemory();
}