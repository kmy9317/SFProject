#include "SFHeroAnimationData.h"

FSFMontagePlayData FSFDirectionalMontageData::GetByDirection(float AngleDegrees) const
{
	// 각도 정규화 (-180 ~ 180)
	AngleDegrees = FMath::Fmod(AngleDegrees + 180.f, 360.f) - 180.f;

	const FSFRandomMontageData* Selected = nullptr;

	// 4방향 판정 (±45도 기준)
	if (AngleDegrees >= -45.f && AngleDegrees < 45.f)
	{
		Selected = &Front;
	}
	else if (AngleDegrees >= 45.f && AngleDegrees < 135.f)
	{
		Selected = &Right;
	}
	else if (AngleDegrees >= -135.f && AngleDegrees < -45.f)
	{
		Selected = &Left;
	}
	else
	{
		Selected = &Back;
	}

	if (Selected && Selected->IsValid())
	{
		return Selected->GetRandom();
	}

	// Fallback: Front
	if (Front.IsValid())
	{
		return Front.GetRandom();
	}

	return FSFMontagePlayData();
}

FSFMontagePlayData USFHeroAnimationData::GetSingleMontage(const FGameplayTag& Tag) const
{
	if (const FSFMontagePlayData* Found = FindByTag(SingleMontages, Tag))
	{
		return *Found;
	}
	return FSFMontagePlayData();
}

FSFMontagePlayData USFHeroAnimationData::GetRandomMontage(const FGameplayTag& Tag) const
{
	if (const FSFRandomMontageData* Found = FindByTag(RandomMontages, Tag))
	{
		return Found->GetRandom();
	}
	return FSFMontagePlayData();
}

FSFMontagePlayData USFHeroAnimationData::GetDirectionalMontage(const FGameplayTag& Tag, float AngleDegrees) const
{
	if (const FSFDirectionalMontageData* Found = FindByTag(DirectionalMontages, Tag))
	{
		return Found->GetByDirection(AngleDegrees);
	}
	return FSFMontagePlayData();
}
