#include "SFGE_InternalInvincibility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/Attributes/SFCombatSet.h" // 방어력 속성을 위해 필수

// === 1. 무적 GE 설정 ===
USFGE_InternalInvincibility::USFGE_InternalInvincibility()
{
	// 지속 시간: 무한 (노티파이가 끝나면 수동으로 제거할 것이므로)
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// 모디파이어: 방어력(Defense)에 +9,999,999 추가
	FGameplayModifierInfo DefenseMod;
	DefenseMod.Attribute = USFCombatSet::GetDefenseAttribute(); // 기존 CombatSet의 방어력 가져오기
	DefenseMod.ModifierOp = EGameplayModOp::Additive;
	DefenseMod.ModifierMagnitude = FScalableFloat(9999999.0f); // 뚫을 수 없는 방어력

	Modifiers.Add(DefenseMod);
}

// === 2. 노티파이 시작 (무적 적용) ===
void USFAnimNotifyState_Invincible::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp || !MeshComp->GetOwner()) return;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner());
	if (ASC)
	{
		// C++로 정의한 GE 클래스를 즉석에서 적용
		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddSourceObject(MeshComp->GetOwner());

		// Spec 생성
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(USFGE_InternalInvincibility::StaticClass(), 1.0f, ContextHandle);
		
		if (SpecHandle.IsValid())
		{
			// 이펙트 적용 및 핸들 저장 (나중에 끄기 위해)
			FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			ActiveEffectHandles.Add(MeshComp, ActiveHandle);
		}
	}
}

// === 3. 노티파이 종료 (무적 해제) ===
void USFAnimNotifyState_Invincible::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp || !MeshComp->GetOwner()) return;

	// 저장해둔 핸들을 찾아 제거
	if (FActiveGameplayEffectHandle* FoundHandle = ActiveEffectHandles.Find(MeshComp))
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner());
		if (ASC)
		{
			ASC->RemoveActiveGameplayEffect(*FoundHandle);
		}
		ActiveEffectHandles.Remove(MeshComp);
	}
}