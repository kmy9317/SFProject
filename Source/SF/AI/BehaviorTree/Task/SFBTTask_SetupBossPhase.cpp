#include "SFBTTask_SetupBossPhase.h"

#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Animation/AnimInstance.h"

USFBTTask_SetupBossPhase::USFBTTask_SetupBossPhase()
{
	NodeName = TEXT("Setup Boss Phase (v2 Running)");
	bNotifyTick = true;
	
	PhaseInitializedKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(USFBTTask_SetupBossPhase, PhaseInitializedKey));
}

EBTNodeResult::Type USFBTTask_SetupBossPhase::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 0. 중복 실행 방지
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (Blackboard && PhaseInitializedKey.SelectedKeyName != NAME_None)
	{
		if (Blackboard->GetValueAsBool(PhaseInitializedKey.SelectedKeyName))
		{
			return EBTNodeResult::Succeeded;
		}
	}

	ACharacter* OwnerChar = Cast<ACharacter>(OwnerComp.GetAIOwner()->GetPawn());
	if (!OwnerChar) return EBTNodeResult::Failed;

	// -------------------------------------------------------------------------
	// 1. 애니메이션 레이어 교체
	// -------------------------------------------------------------------------
	if (PhaseAnimLayer)
	{
		if (USkeletalMeshComponent* Mesh = OwnerChar->GetMesh())
		{
			Mesh->LinkAnimClassLayers(PhaseAnimLayer);
		}
	}

	// -------------------------------------------------------------------------
	// 2. [핵심] 이동 수치 '권장값' 적용
	// -------------------------------------------------------------------------
	if (UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement())
	{
		// 1) 속도: 600 (Running)
		if (NewMaxWalkSpeed > 0.0f) 
			MoveComp->MaxWalkSpeed = NewMaxWalkSpeed;

		// 2) 가속도: 2400 (즉각적인 출발 반응)
		if (NewMaxAcceleration > 0.0f) 
			MoveComp->MaxAcceleration = NewMaxAcceleration;

		// 3) 제동력: 2048 (칼 같은 정지)
		if (NewBrakingDeceleration > 0.0f) 
			MoveComp->BrakingDecelerationWalking = NewBrakingDeceleration;

		// 4) 회전 속도: 720 (빠른 타겟 추적)
		if (NewRotationRateYaw > 0.0f) 
			MoveComp->RotationRate.Yaw = NewRotationRateYaw;
		
		// 5) 마찰력: 8.0 (미끄러짐 방지)
		if (NewGroundFriction > 0.0f) 
			MoveComp->GroundFriction = NewGroundFriction;
	}

	// -------------------------------------------------------------------------
	// 3. 변신 몽타주 재생
	// -------------------------------------------------------------------------
	if (PhaseStartMontage)
	{
		UAnimInstance* AnimInstance = OwnerChar->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			float Duration = OwnerChar->PlayAnimMontage(PhaseStartMontage);
			
			if (Duration > 0.0f && bWaitForMontageEnd)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &USFBTTask_SetupBossPhase::OnMontageEnded, &OwnerComp);
				AnimInstance->Montage_SetEndDelegate(EndDelegate, PhaseStartMontage);

				bIsPlayingMontage = true;
				return EBTNodeResult::InProgress;
			}
		}
	}

	FinishAndSetFlag(OwnerComp, EBTNodeResult::Succeeded);
	return EBTNodeResult::Succeeded;
}

void USFBTTask_SetupBossPhase::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	if (bIsPlayingMontage)
	{
		ACharacter* OwnerChar = Cast<ACharacter>(OwnerComp.GetAIOwner()->GetPawn());
		if (OwnerChar)
		{
			UAnimInstance* AnimInstance = OwnerChar->GetMesh()->GetAnimInstance();
			// 몽타주가 끝났거나 중단되었는지 확인
			if (!AnimInstance || !AnimInstance->Montage_IsPlaying(PhaseStartMontage))
			{
				FinishAndSetFlag(OwnerComp, EBTNodeResult::Succeeded);
				bIsPlayingMontage = false;
			}
		}
		else
		{
			FinishAndSetFlag(OwnerComp, EBTNodeResult::Failed);
			bIsPlayingMontage = false;
		}
	}
}

void USFBTTask_SetupBossPhase::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted, UBehaviorTreeComponent* OwnerComp)
{
	if (OwnerComp && bIsPlayingMontage)
	{
		FinishAndSetFlag(*OwnerComp, EBTNodeResult::Succeeded);
		bIsPlayingMontage = false;
	}
}

void USFBTTask_SetupBossPhase::FinishAndSetFlag(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type Result)
{
	if (Result == EBTNodeResult::Succeeded)
	{
		UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
		if (Blackboard && PhaseInitializedKey.SelectedKeyName != NAME_None)
		{
			Blackboard->SetValueAsBool(PhaseInitializedKey.SelectedKeyName, true);
		}
	}
	
	if (OwnerComp.GetActiveNode() == this) 
	{
		FinishLatentTask(OwnerComp, Result);
	}
}