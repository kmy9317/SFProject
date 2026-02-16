#include "SFGA_Upgrade_Skill_Token.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Hero_SkillTypeChange.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"

USFGA_Upgrade_Skill_Token::USFGA_Upgrade_Skill_Token()
{
	// 패시브처럼 동작하며, 인스턴싱되어야 저장/로드가 가능
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	bShouldPersistOnTravel = false;
}

void USFGA_Upgrade_Skill_Token::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

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

	// 관리자 GA 찾기 → 오버라이드 등록
	for (const FGameplayAbilitySpec& AbilitySpec : SFASC->GetActivatableAbilities())
	{
		if (AbilitySpec.Ability && AbilitySpec.Ability->IsA(USFGA_Hero_SkillTypeChange::StaticClass()))
		{
			if (USFGA_Hero_SkillTypeChange* ManagerGA = Cast<USFGA_Hero_SkillTypeChange>(AbilitySpec.GetPrimaryInstance()))
			{
				ManagerGA->RegisterSkillOverride(TargetElementTag, TargetInputTag, NewAbilityClass);
			}
			break;
		}
	}

	// 역할 완료 → ASC에서 자기 자신 제거
	SFASC->ClearAbility(Spec.Handle);
}