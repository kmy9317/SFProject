#include "SFPermanentUpgradeComponent.h"

#include "AbilitySystemComponent.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "System/SFPermanentUpgradeGameplayTags.h"
#include "System/Data/SFPermanentUpgradeTypes.h"
#include "System/Data/SFUpgradeBonusData.h"

DEFINE_LOG_CATEGORY_STATIC(LogPermanentUpgrade, Log, All);

namespace
{
	struct FAccStats
	{
		float MaxHealthPct = FMath::Clamp(MaxHealthPct,  1.f, 20.f);
		float MaxManaPct = FMath::Clamp(MaxManaPct,    1.f, 20.f);
		float MaxStaminaPct = FMath::Clamp(MaxStaminaPct, 1.f, 20.f);
		float CritDamagePct = 0.f;
		float AttackPowerPct = 0.f;
		
		float MaxHealth = 0.f;
		float MaxMana = 0.f;
		float MaxStamina = 0.f;
		float Luck = 0.f;
		float CritChance = 0.f;

		float CooldownReductionPct = 0.f;
		float CurrencyGainPct = 0.f;

		void AddFrom(const FSFUpgradeTierBonus& Bonus, float Mult)
		{
			MaxHealthPct += Bonus.BonusMaxHealthPercent * Mult;
			MaxManaPct += Bonus.BonusMaxManaPercent * Mult;
			MaxStaminaPct += Bonus.BonusMaxStaminaPercent * Mult;
			CritDamagePct += Bonus.BonusCriticalDamagePercent * Mult;
			AttackPowerPct += Bonus.BonusAttackPowerPercent * Mult;

			MaxHealth += Bonus.BonusMaxHealth * Mult;
			MaxMana += Bonus.BonusMaxMana * Mult;
			MaxStamina += Bonus.BonusMaxStamina * Mult;
			Luck += Bonus.BonusLuck * Mult;
			CritChance += Bonus.BonusCriticalChance * Mult;

			CooldownReductionPct += Bonus.BonusCooldownReductionPercent * Mult;
			CurrencyGainPct += Bonus.BonusCurrencyGainPercent * Mult;
		}
	};

	static void SetByCallerIfNonZero(FGameplayEffectSpec& Spec, const FGameplayTag& Tag, float Value)
	{
		// 0도 의미가 있을 수 있지만(테스트/디버그), 불필요한 오버라이드를 피하려면 NonZero가 안전함.
		// 여기서는 "항상 덮어쓰기" 방식이 더 직관적이라 0도 그대로 세팅.
		Spec.SetSetByCallerMagnitude(Tag, Value);
	}
}

USFPermanentUpgradeComponent::USFPermanentUpgradeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USFPermanentUpgradeComponent::BeginPlay()
{
	Super::BeginPlay();
}

FString USFPermanentUpgradeComponent::CategoryToKey(ESFUpgradeCategory Category)
{
	switch (Category)
	{
	case ESFUpgradeCategory::Wrath: return TEXT("wrath");
	case ESFUpgradeCategory::Pride: return TEXT("pride");
	case ESFUpgradeCategory::Lust:  return TEXT("lust");
	case ESFUpgradeCategory::Sloth: return TEXT("sloth");
	case ESFUpgradeCategory::Greed: return TEXT("greed");
	default: return TEXT("unknown");
	}
}

const FSFUpgradeTierBonus* USFPermanentUpgradeComponent::FindTierRow(ESFUpgradeCategory Category, int32 Tier) const
{
	if (!UpgradeBonusTable)
	{
		return nullptr;
	}

	const FString RowKeyStr = FString::Printf(TEXT("%s_T%d"), *CategoryToKey(Category), Tier);
	const FName RowKey(*RowKeyStr);
	return UpgradeBonusTable->FindRow<FSFUpgradeTierBonus>(RowKey, TEXT("PermanentUpgrade"));
}

void USFPermanentUpgradeComponent::ClearUpgradeBonuses(UAbilitySystemComponent* ASC)
{
	UAbilitySystemComponent* TargetASC = ASC ? ASC : AppliedASC.Get();
	if (!TargetASC)
	{
		AppliedASC.Reset();
		StatEffectHandle.Invalidate();
		GrantedAbilityHandles.Reset();
		GrantedLooseTags.Reset();
		GrantedEffectHandles.Reset();
		return;
	}

	if (StatEffectHandle.IsValid())
	{
		TargetASC->RemoveActiveGameplayEffect(StatEffectHandle);
		StatEffectHandle.Invalidate();
	}

	for (const FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
	{
		if (Handle.IsValid())
		{
			TargetASC->ClearAbility(Handle);
		}
	}
	GrantedAbilityHandles.Reset();

	if (!GrantedLooseTags.IsEmpty())
	{
		TargetASC->RemoveLooseGameplayTags(GrantedLooseTags);
		GrantedLooseTags.Reset();
	}

	for (const FActiveGameplayEffectHandle& Handle : GrantedEffectHandles)
	{
		if (Handle.IsValid())
		{
			TargetASC->RemoveActiveGameplayEffect(Handle);
		}
	}
	GrantedEffectHandles.Reset();

	AppliedASC.Reset();
}

void USFPermanentUpgradeComponent::ApplyUpgradeBonuses(UAbilitySystemComponent* ASC, const FSFPermanentUpgradeData& UpgradeData)
{
	if (!ASC)
	{
		return;
	}

	UE_LOG(
	LogTemp,
	Warning,
	TEXT("[PermanentUpgrade] ApplyUpgradeBonuses CALLED")
	);
	
	ClearUpgradeBonuses(ASC);
	AppliedASC = ASC;

	if (!UpgradeBonusTable || !StatBonusEffectClass)
	{
		UE_LOG(LogPermanentUpgrade, Warning, TEXT("ApplyUpgradeBonuses aborted - missing DataTable or StatBonusEffectClass"));
		return;
	}

	FAccStats Acc;

	// 1) Tier 0: 포인트(레벨)당 보너스 (row: <category>_T0)
	for (int32 CatIdx = 0; CatIdx < static_cast<int32>(ESFUpgradeCategory::MAX); ++CatIdx)
	{
		const ESFUpgradeCategory Category = static_cast<ESFUpgradeCategory>(CatIdx);
		const int32 Points = UpgradeData.GetPoints(Category);

		if (const FSFUpgradeTierBonus* PerPoint = FindTierRow(Category, 0))
		{
			Acc.AddFrom(*PerPoint, static_cast<float>(Points));
		}
	}

	// 2) Tier 1~3: 달성 보너스 (row: <category>_T1..T3, Tier 필드와 무관하게 RowKey로 조회)
	for (int32 CatIdx = 0; CatIdx < static_cast<int32>(ESFUpgradeCategory::MAX); ++CatIdx)
	{
		const ESFUpgradeCategory Category = static_cast<ESFUpgradeCategory>(CatIdx);
		const int32 CurrentTier = UpgradeData.GetCurrentTier(Category);

		for (int32 Tier = 1; Tier <= CurrentTier; ++Tier)
		{
			if (const FSFUpgradeTierBonus* TierBonus = FindTierRow(Category, Tier))
			{
				Acc.AddFrom(*TierBonus, 1.f);
			}
		}
	}

	// 3) 누적 스탯을 SetByCaller로 GE에 전달
	FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(StatBonusEffectClass, 1.f, Ctx);
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogPermanentUpgrade, Warning, TEXT("Failed to create StatBonusEffect spec"));
		return;
	}

	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
	check(Spec);

	SetByCallerIfNonZero(*Spec, SFPermanentUpgradeTags::Data_Upgrade_MaxHealthPct, Acc.MaxHealthPct);
	SetByCallerIfNonZero(*Spec, SFPermanentUpgradeTags::Data_Upgrade_MaxManaPct, Acc.MaxManaPct);
	SetByCallerIfNonZero(*Spec, SFPermanentUpgradeTags::Data_Upgrade_MaxStaminaPct, Acc.MaxStaminaPct);
	SetByCallerIfNonZero(*Spec, SFPermanentUpgradeTags::Data_Upgrade_CritDamagePct, Acc.CritDamagePct);
	SetByCallerIfNonZero(*Spec, SFPermanentUpgradeTags::Data_Upgrade_AttackPowerPct, Acc.AttackPowerPct);

	SetByCallerIfNonZero(*Spec, SFPermanentUpgradeTags::Data_Upgrade_MaxHealth, Acc.MaxHealth);
	SetByCallerIfNonZero(*Spec, SFPermanentUpgradeTags::Data_Upgrade_MaxMana, Acc.MaxMana);
	SetByCallerIfNonZero(*Spec, SFPermanentUpgradeTags::Data_Upgrade_MaxStamina, Acc.MaxStamina);
	SetByCallerIfNonZero(*Spec, SFPermanentUpgradeTags::Data_Upgrade_Luck, Acc.Luck);
	SetByCallerIfNonZero(*Spec, SFPermanentUpgradeTags::Data_Upgrade_CritChance, Acc.CritChance);

	SetByCallerIfNonZero(*Spec, SFPermanentUpgradeTags::Data_Upgrade_CooldownReductionPct, Acc.CooldownReductionPct);
	SetByCallerIfNonZero(*Spec, SFPermanentUpgradeTags::Data_Upgrade_CurrencyGainPct, Acc.CurrencyGainPct);

	StatEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec);

	UE_LOG(LogPermanentUpgrade, Warning, TEXT("AccStats: HP=%.2f MP=%.2f ST=%.2f Crit=%.4f Luck=%.2f"), Acc.MaxHealth, Acc.MaxMana, Acc.MaxStamina, Acc.CritChance, Acc.Luck);

	// 4) Tier 1~3 특수효과(태그/어빌리티/추가 GE)
	for (int32 CatIdx = 0; CatIdx < static_cast<int32>(ESFUpgradeCategory::MAX); ++CatIdx)
	{
		const ESFUpgradeCategory Category = static_cast<ESFUpgradeCategory>(CatIdx);
		const int32 CurrentTier = UpgradeData.GetCurrentTier(Category);

		for (int32 Tier = 1; Tier <= CurrentTier; ++Tier)
		{
			const FSFUpgradeTierBonus* Row = FindTierRow(Category, Tier);
			if (!Row)
			{
				continue;
			}

			if (!Row->GrantedTags.IsEmpty())
			{
				ASC->AddLooseGameplayTags(Row->GrantedTags);
				GrantedLooseTags.AppendTags(Row->GrantedTags);
			}

			for (const TSubclassOf<UGameplayAbility>& AbilityClass : Row->GrantedAbilities)
			{
				if (!AbilityClass)
				{
					continue;
				}

				FGameplayAbilitySpec SpecToGive(AbilityClass, 1, INDEX_NONE, GetOwner());
				const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(SpecToGive);
				GrantedAbilityHandles.Add(Handle);
			}

			if (Row->BonusEffect)
			{
				FGameplayEffectSpecHandle BonusSpecHandle = ASC->MakeOutgoingSpec(Row->BonusEffect, 1.f, Ctx);
				if (BonusSpecHandle.IsValid())
				{
					const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*BonusSpecHandle.Data.Get());
					GrantedEffectHandles.Add(Handle);
				}
			}
		}
	}
}
