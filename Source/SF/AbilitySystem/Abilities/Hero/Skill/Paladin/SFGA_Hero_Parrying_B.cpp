#include "AbilitySystem/Abilities/Hero/Skill/Paladin/SFGA_Hero_Parrying_B.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/Hero/Skill/SkillActor/SFBuffArea.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

void USFGA_Hero_Parrying_B::OnParrySuccess(const FGameplayEventData& Payload, AActor* InstigatorActor)
{
	if (!BuffAreaClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !CurrentActorInfo)
	{
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return;
	}

	// BuffArea 액터 소환 위치 지정
	FVector SpawnLoc = Avatar->GetActorLocation();

	float HalfHeight = 0.f;
	if (ACharacter* Char = Cast<ACharacter>(Avatar))
	{
		if (auto* Capsule = Char->GetCapsuleComponent())
		{
			HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	SpawnLoc.Z -= HalfHeight;
	SpawnLoc.Z += 3.f;

	FActorSpawnParameters Params;
	Params.Owner = Avatar;
	Params.Instigator = Cast<APawn>(Avatar);

	ASFBuffArea* AreaActor = World->SpawnActor<ASFBuffArea>(BuffAreaClass, SpawnLoc, FRotator::ZeroRotator, Params);

	if (AreaActor)
	{
		AreaActor->InitializeArea(ASC);
	}
}
