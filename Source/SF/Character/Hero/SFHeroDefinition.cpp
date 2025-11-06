#include "SFHeroDefinition.h"
#include "Character/SFPawnData.h"

UTexture2D* USFHeroDefinition::LoadIcon() const
{
	HeroIcon.LoadSynchronous();
	if (HeroIcon.IsValid())
	{
		return HeroIcon.Get();
	}
	return nullptr;
}

TSubclassOf<UAnimInstance> USFHeroDefinition::LoadDisplayAnimationBP() const
{
	DisplayAnimBP.LoadSynchronous();
	if (DisplayAnimBP.IsValid())
	{
		return DisplayAnimBP.Get();
	}
	return TSubclassOf<UAnimInstance>();
}

USkeletalMesh* USFHeroDefinition::LoadDisplayMesh() const
{
	DisplayMesh.LoadSynchronous();
	if (DisplayMesh.IsValid())
	{
		return DisplayMesh.Get();
	}
	return nullptr;
}

const USFPawnData* USFHeroDefinition::GetPawnData() const
{
	if (!PawnData.IsNull())
	{
		return PawnData.LoadSynchronous();
	}
	return nullptr;
}