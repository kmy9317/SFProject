#include "SFBTS_UpdateTargetData.h"
#include "AI/Controller/Dragon/SFDragonCombatComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

USFBTS_UpdateTargetData::USFBTS_UpdateTargetData()
{
    NodeName = "Update Target Data ";
    Interval = 0.5f;
    RandomDeviation = 0.1f;
    
    BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(USFBTS_UpdateTargetData, BlackboardKey), AActor::StaticClass());
    
    DistanceKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(USFBTS_UpdateTargetData, DistanceKey));
    
    AngleKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(USFBTS_UpdateTargetData, AngleKey));
    
    ZoneKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(USFBTS_UpdateTargetData, ZoneKey), 
        StaticEnum<EBossAttackZone>());

    bNotifyBecomeRelevant = true;
    bNotifyCeaseRelevant = true;
    bNotifyTick = true;
}

void USFBTS_UpdateTargetData::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
    
    if (!CombatComponent) return;
    
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return;

    AActor* NewTarget = CombatComponent->GetCurrentTarget();

    
    if (NewTarget)
    {
        BB->SetValueAsObject(GetSelectedBlackboardKey(), NewTarget);
        BB->SetValueAsFloat(DistanceKey.SelectedKeyName, CombatComponent->GetDistanceToTarget());
        BB->SetValueAsFloat(AngleKey.SelectedKeyName, CombatComponent->GetAngleToTarget());
        BB->SetValueAsEnum(ZoneKey.SelectedKeyName, static_cast<uint8>(CombatComponent->GetTargetLocationZone()));
    }
    else
    {
        BB->SetValueAsObject(GetSelectedBlackboardKey(), nullptr);
        BB->SetValueAsFloat(DistanceKey.SelectedKeyName, 0.0f);
        BB->SetValueAsFloat(AngleKey.SelectedKeyName, 0.0f);
        BB->SetValueAsEnum(ZoneKey.SelectedKeyName, static_cast<uint8>(EBossAttackZone::OutOfRange)); 
    }
}
void USFBTS_UpdateTargetData::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::OnBecomeRelevant(OwnerComp, NodeMemory);
    
    AAIController* AIC = OwnerComp.GetAIOwner();
    if (AIC)
    {
        CombatComponent = AIC->FindComponentByClass<USFDragonCombatComponent>();
    }
}

void USFBTS_UpdateTargetData::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::OnCeaseRelevant(OwnerComp, NodeMemory);
    CombatComponent = nullptr;
}