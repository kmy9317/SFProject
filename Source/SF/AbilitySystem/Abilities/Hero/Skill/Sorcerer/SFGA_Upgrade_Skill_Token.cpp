#include "SFGA_Upgrade_Skill_Token.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Hero_SkillTypeChange.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"

USFGA_Upgrade_Skill_Token::USFGA_Upgrade_Skill_Token()
{
	// 패시브처럼 동작하며, 인스턴싱되어야 저장/로드가 가능
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void USFGA_Upgrade_Skill_Token::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	// 서버에서만 실행
	if (!ActorInfo->IsNetAuthority())
	{
		return;
	}

	USFAbilitySystemComponent* SFASC = Cast<USFAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
	if (!SFASC)
	{
		return;
	}

	// 관리자 GA (SFGA_Hero_SkillTypeChange) 찾기
	USFGA_Hero_SkillTypeChange* ManagerGA = nullptr;
	for (const FGameplayAbilitySpec& AbilitySpec : SFASC->GetActivatableAbilities())
	{
		if (AbilitySpec.Ability && AbilitySpec.Ability->IsA(USFGA_Hero_SkillTypeChange::StaticClass()))
		{
			ManagerGA = Cast<USFGA_Hero_SkillTypeChange>(AbilitySpec.GetPrimaryInstance());
			break;
		}
	}

	// 관리자에게 스킬 교체 요청
	if (ManagerGA)
	{
		ManagerGA->RegisterSkillOverride(TargetElementTag, TargetInputTag, NewAbilityClass);
	}
}