#include "SFBTTask_FaceTarget.h"
#include "AI/Controller/SFBaseAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "Animation/Enemy/SFEnemyAnimInstance.h"

USFBTTask_FaceTarget::USFBTTask_FaceTarget()
{
    NodeName = "SF Observe Face Target";
    bNotifyTick = true;   // Tick 사용
    bNotifyTaskFinished = true;
}

EBTNodeResult::Type USFBTTask_FaceTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 항상 InProgress 반환 -> Tick에서 종료 처리
    return EBTNodeResult::InProgress;
}

void USFBTTask_FaceTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    ASFBaseAIController* AIController = Cast<ASFBaseAIController>(OwnerComp.GetAIOwner());
    if (!AIController || !AIController->GetPawn())
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    ACharacter* Character = Cast<ACharacter>(AIController->GetPawn());
    if (!Character)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    AActor* TargetActor = GetTargetFromBlackboard(OwnerComp);
    if (!TargetActor)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // FaceTarget 호출 (ControllerYaw 모드에서 ControlRotation 보간)
    

    // AnimInstance 상태 확인
    bool bReady = true;
    if (USFEnemyAnimInstance* Anim = Cast<USFEnemyAnimInstance>(Character->GetMesh()->GetAnimInstance()))
    {
        // TurnInPlace 중이면 아직 완료되지 않음
        bReady = !Anim->IsTurningInPlace();
    }

    // 각도 확인 (AcceptableAngle 이내면 완료)
    float DeltaYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(
        Character->GetActorRotation().Yaw,
        AIController->GetControlRotation().Yaw
    ));

    if (DeltaYaw > AcceptableAngle)
        bReady = false;

    if (bReady)
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
}

EBTNodeResult::Type USFBTTask_FaceTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // Task 중단 시 회전 모드를 ControllerYaw로 복귀
    if (ASFBaseAIController* AIController = Cast<ASFBaseAIController>(OwnerComp.GetAIOwner()))
    {
        AIController->SetRotationMode(EAIRotationMode::ControllerYaw);
    }

    return EBTNodeResult::Failed;
}
AActor* USFBTTask_FaceTarget::GetTargetFromBlackboard(UBehaviorTreeComponent& OwnerComp) const
{
    if (UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent())
    {
        UObject* TargetObj = BlackboardComp->GetValueAsObject(BlackboardKey.SelectedKeyName);
        if (AActor* TargetActor = Cast<AActor>(TargetObj))
        {
            return TargetActor;
        }
    }
    return nullptr;
}

