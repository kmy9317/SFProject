#include "SFBTTask_FaceTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"
#include "AI/Controller/SFBaseAIController.h"
#include "Animation/Enemy/SFEnemyAnimInstance.h"
#include "GameFramework/Character.h"


USFBTTask_FaceTarget::USFBTTask_FaceTarget()
{
    NodeName = "SF Face Target";
    bNotifyTick = true;
    bNotifyTaskFinished = true;

    AngleKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(USFBTTask_FaceTarget, AngleKey));
}

float USFBTTask_FaceTarget::CalculateAngleToTarget(APawn* InPawn, AActor* InTarget, UBehaviorTreeComponent& OwnerComp)
{
    if (bUseAngleKey && AngleKey.IsSet())
    {
        return OwnerComp.GetBlackboardComponent()->GetValueAsFloat(AngleKey.SelectedKeyName);
    }

    if (!InPawn || !InTarget)
        return 0.0f;

    FVector Forward = InPawn->GetActorForwardVector().GetSafeNormal2D();
    FVector ToTarget = (InTarget->GetActorLocation() - InPawn->GetActorLocation()).GetSafeNormal2D();

    if (ToTarget.IsNearlyZero())
        return 0.0f;

    float Dot = FVector::DotProduct(Forward, ToTarget);
    float CalculatedAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

    return CalculatedAngle;
}

float USFBTTask_FaceTarget::CalculateAngleToTarget_Control(APawn* InPawn, AActor* InTarget, AAIController* AIC)
{
    if (!InPawn || !InTarget || !AIC)
        return 0.0f;

    FVector ToTarget = (InTarget->GetActorLocation() - InPawn->GetActorLocation()).GetSafeNormal2D();
    if (ToTarget.IsNearlyZero())
        return 0.0f;

    const FRotator ControlRot = AIC->GetControlRotation();
    const FVector ControlForward = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::X).GetSafeNormal2D();

    float Dot = FVector::DotProduct(ControlForward, ToTarget);
    return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
}

void USFBTTask_FaceTarget::SyncControlRotationToPawn(ASFBaseAIController* AIC)
{
    if (!AIC)
        return;

    ACharacter* Character = AIC->GetCharacter();
    if (Character)
    {
        FRotator CurrentRot = Character->GetActorRotation();
        AIC->SetControlRotation(FRotator(0.f, CurrentRot.Yaw, 0.f));
    }
}

EBTNodeResult::Type USFBTTask_FaceTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    ASFBaseAIController* AIC = Cast<ASFBaseAIController>(OwnerComp.GetAIOwner());
    if (!AIC || !AIC->GetPawn())
        return EBTNodeResult::Failed;

    AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(BlackboardKey.SelectedKeyName));
    if (!Target)
        return EBTNodeResult::Failed;

    AIC->StopMovement();
    AIC->SetFocus(Target, EAIFocusPriority::Gameplay);
    AIC->TargetActor = Target;

    if (AIC->GetCurrentRotationMode() != EAIRotationMode::ControllerYaw)
    {
        AIC->SetRotationMode(EAIRotationMode::ControllerYaw);
    }

    float ControlAngle = CalculateAngleToTarget_Control(AIC->GetPawn(), Target, AIC);
    if (ControlAngle <= AcceptableAngle)
    {
        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::InProgress;
}

void USFBTTask_FaceTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    ASFBaseAIController* AIC = Cast<ASFBaseAIController>(OwnerComp.GetAIOwner());
    if (!AIC || !AIC->TargetActor || !AIC->GetPawn())
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // ControlRotation 기준으로 논리적 회전 완료 판단
    float ControlAngle = CalculateAngleToTarget_Control(AIC->GetPawn(), AIC->TargetActor, AIC);
    bool bControlAligned = (ControlAngle <= AcceptableAngle);

    // AnimInstance에서 시각적 회전 완료 판단
    bool bAnimComplete = true;
    float RemainingYaw = 0.0f;

    if (ACharacter* Character = Cast<ACharacter>(AIC->GetPawn()))
    {
        if (USFEnemyAnimInstance* AnimInstance = Cast<USFEnemyAnimInstance>(Character->GetMesh()->GetAnimInstance()))
        {
            bAnimComplete = !AnimInstance->IsTurningInPlace();
            RemainingYaw = AnimInstance->GetRemainingTurnYaw();

            if (RemainingYaw > AcceptableAngle)
            {
                bAnimComplete = false;
            }
        }
    }

    // 논리적 완료 + 시각적 완료 둘 다 만족해야 성공
    if (bControlAligned && bAnimComplete)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}


void USFBTTask_FaceTarget::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

EBTNodeResult::Type USFBTTask_FaceTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    return EBTNodeResult::Aborted;
}