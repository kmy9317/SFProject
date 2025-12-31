// Fill out your copyright notice in the Description page of Project Settings.

#include "SFBTTask_ApproachAndAttack.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "Character/SFCharacterGameplayTags.h" // 프로젝트 태그 헤더
#include "Navigation/PathFollowingComponent.h"
#include "Kismet/KismetMathLibrary.h" // 회전 계산용

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
	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}

	// 1. 회전 필요 여부 체크 (Rotate Phase Check)
	FVector ToTarget = TargetActor->GetActorLocation() - OwnerPawn->GetActorLocation();
	ToTarget.Z = 0.0f; // 높이 무시

	if (!ToTarget.IsNearlyZero())
	{
		FRotator TargetRot = ToTarget.Rotation();
		FRotator CurrentRot = OwnerPawn->GetActorRotation();
		
		// 각도 차이 계산 (Yaw)
		float DeltaYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRot.Yaw, TargetRot.Yaw));

		if (DeltaYaw > FacingThreshold)
		{
			// 각도가 크면 회전부터 시작
			bIsRotating = true;
			// 이동 중지 (제자리 회전)
			AIController->StopMovement();
			return EBTNodeResult::InProgress;
		}
	}

	// 회전이 필요 없으면 바로 이동/공격 단계로 진입
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

	// [안전장치] 타임아웃
	if (MaxDuration > 0.0f && ElapsedTime >= MaxDuration)
	{
		bFinished = true;
		AIController->StopMovement();
		CleanupDelegate(OwnerComp);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 1단계: 회전 (Rotating Phase)
	if (bIsRotating)
	{
		FVector ToTarget = TargetActor->GetActorLocation() - OwnerPawn->GetActorLocation();
		ToTarget.Z = 0.0f;
		
		if (!ToTarget.IsNearlyZero())
		{
			FRotator TargetRot = ToTarget.Rotation();
			FRotator CurrentRot = OwnerPawn->GetActorRotation();

			// 부드럽게 회전 (RInterpTo)
			FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaSeconds, RotationSpeed);
			OwnerPawn->SetActorRotation(NewRot);

			// 각도 체크
			float DeltaYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(NewRot.Yaw, TargetRot.Yaw));
			if (DeltaYaw <= FacingThreshold)
			{
				// 충분히 바라보았음 -> 다음 단계(이동/공격)로 전환
				bIsRotating = false;
				StartApproachOrAttack(OwnerComp);
			}
		}
		else
		{
			// 타겟과 겹쳐있으면 회전 종료
			bIsRotating = false;
			StartApproachOrAttack(OwnerComp);
		}
	}
	// 2단계: 이동 (Moving Phase)
	else if (bIsMoving)
	{
		float DistSq = FVector::DistSquared(OwnerPawn->GetActorLocation(), TargetActor->GetActorLocation());
		float AttackRadiusSq = AttackRadius * AttackRadius;

		// 사거리 안에 들어왔는지 체크
		if (DistSq <= AttackRadiusSq)
		{
			bIsMoving = false;
			AIController->StopMovement(); // 멈추고
			PerformAttack(OwnerComp);     // 공격 (3단계)
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
		float DistSq = FVector::DistSquared(OwnerPawn->GetActorLocation(), TargetActor->GetActorLocation());
		float AttackRadiusSq = AttackRadius * AttackRadius;

		if (DistSq <= AttackRadiusSq)
		{
			// 이미 사거리 안 -> 공격 (3단계)
			PerformAttack(OwnerComp);
		}
		else
		{
			// 사거리 밖 -> 이동 시작 (2단계)
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

	// [수정됨] 여기서는 더 이상 강제 회전을 하지 않습니다. 
	// (이미 앞 단계에서 회전을 완료하고 이동했기 때문)

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