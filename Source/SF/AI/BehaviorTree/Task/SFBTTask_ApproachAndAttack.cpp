// SF/AI/BehaviorTree/Task/SFBTTask_ApproachAndAttack.cpp

#include "SFBTTask_ApproachAndAttack.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "Character/SFCharacterGameplayTags.h" // 프로젝트 태그 헤더
#include "Navigation/PathFollowingComponent.h"
#include "Kismet/KismetMathLibrary.h" // 회전 계산용

// [추가] 태그 인터페이스 (죽음 확인용)
#include "GameplayTagAssetInterface.h"

USFBTTask_ApproachAndAttack::USFBTTask_ApproachAndAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = "SF Approach And Attack";
	bNotifyTick = true; 
	bNotifyTaskFinished = true;
	bCreateNodeInstance = true; 
}

UAbilitySystemComponent* USFBTTask_ApproachAndAttack::GetASC(UBehaviorTreeComponent& OwnerComp) const
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (APawn* Pawn = AIController->GetPawn())
		{
			return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
		}
	}
	return nullptr;
}

// [추가] 타겟이 살아있는지 확인하는 함수
bool USFBTTask_ApproachAndAttack::IsTargetAlive(AActor* Target) const
{
	if (!IsValid(Target)) return false;

	// IGameplayTagAssetInterface를 통해 태그 확인
	const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Target);
	if (TagInterface)
	{
		// Character.State.Dead 태그가 있으면 죽은 것 -> false 반환
		if (TagInterface->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
		{
			return false;
		}
	}
	// 태그 인터페이스가 없으면 일단 살아있다고 가정 (일반 액터 등)
	return true;
}

EBTNodeResult::Type USFBTTask_ApproachAndAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* OwnerPawn = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	if (!AIController || !OwnerPawn || !BB || !AbilityClassToRun)
	{
		return EBTNodeResult::Failed;
	}

	CachedOwnerComp = &OwnerComp;
	bFinished = false;
	bIsRotating = false;
	bIsMoving = false;
	bIsAttacking = false;
	ElapsedTime = 0.0f;

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));

	// [수정] 시작할 때 타겟이 없거나 죽어있으면 즉시 실패 처리
	if (!TargetActor || !IsTargetAlive(TargetActor))
	{
		return EBTNodeResult::Failed;
	}

	// 1. 회전 필요 여부 체크
	FVector ToTarget = TargetActor->GetActorLocation() - OwnerPawn->GetActorLocation();
	ToTarget.Z = 0.0f; 

	if (!ToTarget.IsNearlyZero())
	{
		FRotator TargetRot = ToTarget.Rotation();
		FRotator CurrentRot = OwnerPawn->GetActorRotation();
		
		float DeltaYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRot.Yaw, TargetRot.Yaw));

		if (DeltaYaw > FacingThreshold)
		{
			bIsRotating = true;
			AIController->StopMovement();
			return EBTNodeResult::InProgress;
		}
	}

	StartApproachOrAttack(OwnerComp);

	return EBTNodeResult::InProgress;
}

void USFBTTask_ApproachAndAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	if (bFinished) return;

	ElapsedTime += DeltaSeconds;
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* OwnerPawn = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = BB ? Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName)) : nullptr;

	if (!AIController || !OwnerPawn || !TargetActor)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// [수정] 이동 중 타겟이 죽으면 즉시 중단 (공격 중일 때는 제외)
	if (!bIsAttacking && !IsTargetAlive(TargetActor))
	{
		AIController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 타임아웃 체크
	if (MaxDuration > 0.0f && ElapsedTime >= MaxDuration)
	{
		bFinished = true;
		AIController->StopMovement();
		CleanupDelegate(OwnerComp);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 1단계: 회전
	if (bIsRotating)
	{
		FVector ToTarget = TargetActor->GetActorLocation() - OwnerPawn->GetActorLocation();
		ToTarget.Z = 0.0f;
		
		if (!ToTarget.IsNearlyZero())
		{
			FRotator TargetRot = ToTarget.Rotation();
			FRotator CurrentRot = OwnerPawn->GetActorRotation();

			FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaSeconds, RotationSpeed);
			OwnerPawn->SetActorRotation(NewRot);

			float DeltaYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(NewRot.Yaw, TargetRot.Yaw));
			if (DeltaYaw <= FacingThreshold)
			{
				bIsRotating = false;
				StartApproachOrAttack(OwnerComp);
			}
		}
		else
		{
			bIsRotating = false;
			StartApproachOrAttack(OwnerComp);
		}
	}
	// 2단계: 이동
	else if (bIsMoving)
	{
		float DistSq = FVector::DistSquared(OwnerPawn->GetActorLocation(), TargetActor->GetActorLocation());
		float AttackRadiusSq = AttackRadius * AttackRadius;

		if (DistSq <= AttackRadiusSq)
		{
			bIsMoving = false;
			AIController->StopMovement(); 
			PerformAttack(OwnerComp);     
		}
	}
}

void USFBTTask_ApproachAndAttack::StartApproachOrAttack(UBehaviorTreeComponent& OwnerComp)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* OwnerPawn = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = BB ? Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName)) : nullptr;

	if (OwnerPawn && TargetActor)
	{
		// [수정] 최종 공격/이동 결정 전 한번 더 생존 확인
		if (!IsTargetAlive(TargetActor))
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}

		float DistSq = FVector::DistSquared(OwnerPawn->GetActorLocation(), TargetActor->GetActorLocation());
		float AttackRadiusSq = AttackRadius * AttackRadius;

		if (DistSq <= AttackRadiusSq)
		{
			PerformAttack(OwnerComp);
		}
		else
		{
			bIsMoving = true;
			AIController->MoveToActor(TargetActor, AttackRadius * 0.5f, true, true, false, 0, true);
		}
	}
}

void USFBTTask_ApproachAndAttack::PerformAttack(UBehaviorTreeComponent& OwnerComp)
{
	UAbilitySystemComponent* ASC = GetASC(OwnerComp);
	if (!ASC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FGameplayAbilitySpec* FoundSpec = nullptr;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetClass() == AbilityClassToRun)
		{
			FoundSpec = const_cast<FGameplayAbilitySpec*>(&Spec);
			break;
		}
	}

	if (!FoundSpec || !ASC->TryActivateAbility(FoundSpec->Handle))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	bIsAttacking = true;

	// 태그 대기 (기본값 설정)
	TagToWait = WaitForTag.IsValid() ? WaitForTag : SFGameplayTags::Character_State_Attacking;

	if (ASC->GetTagCount(TagToWait) > 0)
	{
		if (!EventHandle.IsValid())
		{
			EventHandle = ASC->RegisterGameplayTagEvent(TagToWait, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &USFBTTask_ApproachAndAttack::OnTagChanged);
		}
	}
	else
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

void USFBTTask_ApproachAndAttack::OnTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (bFinished) return;

	if (NewCount == 0)
	{
		bFinished = true;
		if (UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get())
		{
			CleanupDelegate(*OwnerComp);
			FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
		}
	}
}

void USFBTTask_ApproachAndAttack::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	CleanupDelegate(OwnerComp);
	CachedOwnerComp.Reset();
	bIsRotating = false;
	bIsMoving = false;
	bIsAttacking = false;
	
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void USFBTTask_ApproachAndAttack::CleanupDelegate(UBehaviorTreeComponent& OwnerComp)
{
	if (EventHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetASC(OwnerComp))
		{
			ASC->UnregisterGameplayTagEvent(EventHandle, TagToWait, EGameplayTagEventType::NewOrRemoved);
		}
	}
	EventHandle.Reset();
}