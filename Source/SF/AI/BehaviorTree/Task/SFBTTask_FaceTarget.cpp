#include "SFBTTask_FaceTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h" // [중요] 헤더 추가

USFBTTask_FaceTarget::USFBTTask_FaceTarget()
{
	NodeName = "SF Rotate to Target (Use Ability)";
	bNotifyTick = true;
}

// [핵심 로직] 어빌리티 인스턴스를 찾아 함수 호출
bool USFBTTask_FaceTarget::CheckAbilityAttackAngle(UBehaviorTreeComponent& OwnerComp, AActor* Target) const
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return false;

	// 1. 블랙보드에서 태그 이름 가져오기
	FName TagName = BB->GetValueAsName(AbilityTagKey.SelectedKeyName);

	// 이름이 없으면(None) 변환 시도조차 하지 않고 리턴 (여기서 터지는 것 방지)
	if (TagName.IsNone())
	{
		return false;
	}

	// 두 번째 인자에 false 전달 (존재하지 않는 태그여도 에러 내지 않고 조용히 처리)
	FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(TagName, false);

	// 태그가 없으면 -> 기본 정밀도(DefaultPrecision)로 직접 계산
	if (!AbilityTag.IsValid()) 
	{
		return false;
	}

	// 2. ASC 가져오기
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return false;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AIC->GetPawn());
	if (!ASC) return false;

	// 3. 해당 태그를 가진 '활성화 가능한 어빌리티' 찾기
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->AbilityTags.HasTag(AbilityTag))
		{
			// 4. 어빌리티 인스턴스(Instance) 가져오기
			USFGA_Enemy_BaseAttack* EnemyAttack = Cast<USFGA_Enemy_BaseAttack>(Spec.GetPrimaryInstance());
			
			// 인스턴스가 없으면 라도 사용 시도
			if (!EnemyAttack) 
			{
				EnemyAttack = Cast<USFGA_Enemy_BaseAttack>(Spec.Ability);
			}

			if (EnemyAttack)
			{
				// 어빌리티가 직접 판단
				return EnemyAttack->IsWithinAttackAngle(Target);
			}
		}
	}

	return false;
}

EBTNodeResult::Type USFBTTask_FaceTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(BlackboardKey.SelectedKeyName));

	if (!AIC || !Target) return EBTNodeResult::Failed;

	// 1. 회전 명령 (엔진 기능)
	AIC->SetFocus(Target);

	// 2. 어빌리티에게 "지금 쏴도 되니?" 물어보기
	if (CheckAbilityAttackAngle(OwnerComp, Target))
	{
		return EBTNodeResult::Succeeded;
	}

	// 3. 어빌리티가 없거나 아직 각도가 안 맞으면 -> 기본 정밀도로 백업 체크
	// (만약 어빌리티가 없는 평타 공격 등일 경우를 대비)
	FVector MyLoc = AIC->GetPawn()->GetActorLocation();
	FVector Dir = (Target->GetActorLocation() - MyLoc).GetSafeNormal2D();
	FVector Forward = AIC->GetPawn()->GetActorForwardVector();
	float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Forward, Dir), -1.f, 1.f)));

	if (Angle <= DefaultPrecision)
	{
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::InProgress;
}

void USFBTTask_FaceTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(BlackboardKey.SelectedKeyName));

	if (!AIC || !Target)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 1. 어빌리티 체크 (우선순위 1)
	if (CheckAbilityAttackAngle(OwnerComp, Target))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 2. 기본 정밀도 체크 (우선순위 2 - 백업)
	FVector MyLoc = AIC->GetPawn()->GetActorLocation();
	FVector Dir = (Target->GetActorLocation() - MyLoc).GetSafeNormal2D();
	FVector Forward = AIC->GetPawn()->GetActorForwardVector();
	float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Forward, Dir), -1.f, 1.f)));

	if (Angle <= DefaultPrecision)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}