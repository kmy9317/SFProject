#include "BTD_CheckHealthRatio.h"
#include "AIController.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
// [중요] 사용자의 AttributeSet 헤더 포함
#include "AbilitySystem/Attributes/SFPrimarySet.h" 

UBTD_CheckHealthRatio::UBTD_CheckHealthRatio()
{
	NodeName = TEXT("Check Health Ratio");
	// FlowAbortMode는 기본적으로 None이지만, 체력이 깎이는 순간 즉시 반응하려면 Self나 Priority로 설정하는 것이 좋습니다.
	FlowAbortMode = EBTFlowAbortMode::None; 
}

bool UBTD_CheckHealthRatio::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return false;

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn) return false;

	// GAS 컴포넌트 가져오기
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	if (!ASC) return false;

	// [수정됨] SFPrimarySet에 정의된 Health 속성값을 안전하게 가져옵니다.
	// HasAttribute() 체크는 안전을 위해 추가했습니다.
	if (!ASC->HasAttributeSetForAttribute(USFPrimarySet::GetHealthAttribute()))
	{
		return false;
	}

	// GetNumericAttribute 함수는 해당 Attribute의 현재 값(Current Value)을 반환합니다.
	const float CurrentHealth = ASC->GetNumericAttribute(USFPrimarySet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(USFPrimarySet::GetMaxHealthAttribute());

	// 0으로 나누기 방지
	if (MaxHealth <= 0.0f)
	{
		return false;
	}

	// 현재 비율 계산
	const float CurrentRatio = CurrentHealth / MaxHealth;

	// 설정한 임계값보다 작거나 같으면 True
	return CurrentRatio <= HealthThreshold;
}

FString UBTD_CheckHealthRatio::GetStaticDescription() const
{
	return FString::Printf(TEXT("Health Ratio <= %.0f%%"), HealthThreshold * 100.f);
}