#include "BTD_CanActivateAbilityByTag.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AI/Controller/SFEnemyController.h"
#include "Character/SFCharacterBase.h"

UBTD_CanActivateAbilityByTag::UBTD_CanActivateAbilityByTag()
{
    NodeName = "Can Activate Ability";
    FlowAbortMode = EBTFlowAbortMode::LowerPriority;
    bNotifyTick = true;
    bNotifyBecomeRelevant = true;
    bNotifyCeaseRelevant = true;
}

uint16 UBTD_CanActivateAbilityByTag::GetInstanceMemorySize() const
{
    return sizeof(FBTCanActivateAbilityMemory);
}

void UBTD_CanActivateAbilityByTag::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
    FBTCanActivateAbilityMemory* Memory = CastInstanceNodeMemory<FBTCanActivateAbilityMemory>(NodeMemory);
    Memory->bLastResult = false;
    Memory->TimeSinceLastCheck = 0.f;
}


bool UBTD_CanActivateAbilityByTag::CalculateRawConditionValue(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory) const
{
    const ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
    if (!AIController)
    {
        return false;
    }

    const ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(AIController->GetPawn());
    if (!SFCharacter)
    {
        return false;
    }

    USFAbilitySystemComponent* ASC = SFCharacter->GetSFAbilitySystemComponent();
    if (!ASC)
    {
        return false;
    }

    const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();
    if (!ActorInfo)
    {
        return false;
    }
    
    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        if (Spec.Ability)
        {
            // 이걸로 ability Tags와 AssetTags를 모두 확인 시키는 플래그 역할 
            bool bTagMatches = false;

            
            for (const FGameplayTag& AbilityTagInSpec : Spec.Ability->AbilityTags)
            {
                if (AbilityTagInSpec.MatchesTag(AbilityTag))
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
                if (Spec.Ability->CanActivateAbility(Spec.Handle, ActorInfo))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void UBTD_CanActivateAbilityByTag::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    FBTCanActivateAbilityMemory* Memory = CastInstanceNodeMemory<FBTCanActivateAbilityMemory>(NodeMemory);
    
    // 같은 bt를 사용할 경우 값이 섞일 수도 있기에 각 ai에게 독립적인 메모리 공간을 준다 
    Memory->TimeSinceLastCheck += DeltaSeconds;
    if (Memory->TimeSinceLastCheck < CheckInterval)
    {
        return;
    }
    Memory->TimeSinceLastCheck = 0.f;

    const bool bCurrentResult = CalculateRawConditionValue(OwnerComp, NodeMemory);
    
    // 값이 바뀌었을 때만 재평가 요청
    if (bCurrentResult != Memory->bLastResult)
    {
        Memory->bLastResult = bCurrentResult;
        OwnerComp.RequestExecution(this);
    }
}