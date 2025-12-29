#include "SFBTD_CompareDistanceWithAbilityRange.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"
#include "Character/SFCharacterBase.h"

USFBTD_CompareDistanceWithAbilityRange::USFBTD_CompareDistanceWithAbilityRange()
{
    NodeName = "Compare Distance With Ability Range";
    bNotifyTick = true;
    Operator = EArithmeticKeyOperation::Greater;

    DistanceKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(USFBTD_CompareDistanceWithAbilityRange, DistanceKey));
    DistanceKey.AllowNoneAsValue(true);  

    AbilityTagKey.AddNameFilter(this, GET_MEMBER_NAME_CHECKED(USFBTD_CompareDistanceWithAbilityRange, AbilityTagKey));
    AbilityTagKey.AllowNoneAsValue(true);  

    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(USFBTD_CompareDistanceWithAbilityRange, TargetKey), AActor::StaticClass());

    FlowAbortMode = EBTFlowAbortMode::Self;
}

uint16 USFBTD_CompareDistanceWithAbilityRange::GetInstanceMemorySize() const
{
    return sizeof(FBTDistanceCompareMemory);
}

void USFBTD_CompareDistanceWithAbilityRange::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
    FBTDistanceCompareMemory* Memory = CastInstanceNodeMemory<FBTDistanceCompareMemory>(NodeMemory);
    Memory->bLastResult = false;
}

bool USFBTD_CompareDistanceWithAbilityRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return false;

    // Target 없으면 실패
    AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetKey.SelectedKeyName));
    APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
    if (!Target || !Pawn) return false;

    // 거리 계산: DistanceKey 우선, 없으면 Pawn ↔ Target 거리
    float Distance = 0.f;
    if (DistanceKey.IsSet())
    {
        Distance = BB->GetValueAsFloat(DistanceKey.SelectedKeyName);
    }
    else
    {
        Distance = CalculateDistance(OwnerComp);
    }

    // Ability Range 가져오기
    float AttackRange = 0.f;

    // AbilityTagKey가 설정되어 있으면 해당 어빌리티의 범위 사용
    if (AbilityTagKey.IsSet())
    {
        const FName TagName = BB->GetValueAsName(AbilityTagKey.SelectedKeyName);
        if (!TagName.IsNone())
        {
            const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(TagName);
            if (AbilityTag.IsValid())
            {
                AttackRange = GetAbilityAttackRange(OwnerComp, AbilityTag);
            }
        }
    }

    // AbilityTagKey가 없거나 범위를 못 가져온 경우 실패
    if (AttackRange <= 0.f) return false;

    // 연산 비교
    bool bResult = false;
    switch (Operator)
    {
    case EArithmeticKeyOperation::Equal:
        bResult = FMath::IsNearlyEqual(Distance, AttackRange, 10.f);
        break;
    case EArithmeticKeyOperation::NotEqual:
        bResult = !FMath::IsNearlyEqual(Distance, AttackRange, 10.f);
        break;
    case EArithmeticKeyOperation::Less:
        bResult = Distance < AttackRange;
        break;
    case EArithmeticKeyOperation::LessOrEqual:
        bResult = Distance <= AttackRange;
        break;
    case EArithmeticKeyOperation::Greater:
        bResult = Distance > AttackRange;
        break;
    case EArithmeticKeyOperation::GreaterOrEqual:
        bResult = Distance >= AttackRange;
        break;
    default:
        break;
    }

    return bResult;
}

void USFBTD_CompareDistanceWithAbilityRange::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    FBTDistanceCompareMemory* Memory = CastInstanceNodeMemory<FBTDistanceCompareMemory>(NodeMemory);

    const bool bCurrentResult = CalculateRawConditionValue(OwnerComp, NodeMemory);
    if (Memory->bLastResult != bCurrentResult)
    {
        Memory->bLastResult = bCurrentResult;
        OwnerComp.RequestExecution(this);
    }
}

float USFBTD_CompareDistanceWithAbilityRange::CalculateDistance(UBehaviorTreeComponent& OwnerComp) const
{
    const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return 0.f;

    AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetKey.SelectedKeyName));
    APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
    if (!Target || !Pawn) return 0.f;

    return FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation());
}

float USFBTD_CompareDistanceWithAbilityRange::GetAbilityAttackRange(UBehaviorTreeComponent& OwnerComp, const FGameplayTag& AbilityTag) const
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController) return 0.f;

    ASFCharacterBase* Character = Cast<ASFCharacterBase>(AIController->GetPawn());
    if (!Character) return 0.f;

    UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Character);
    if (!ASC) return 0.f;

    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        if (!Spec.Ability) continue;

        if (Spec.Ability->AbilityTags.HasTag(AbilityTag) || Spec.Ability->GetAssetTags().HasTag(AbilityTag))
        {
            UGameplayAbility* AbilityInstance = Spec.GetPrimaryInstance();
            if (!AbilityInstance) AbilityInstance = Spec.Ability;

            if (USFGA_Enemy_BaseAttack* BaseAttack = Cast<USFGA_Enemy_BaseAttack>(AbilityInstance))
            {
                return BaseAttack->GetAttackRange();
            }
        }
    }

    return 0.f;
}
