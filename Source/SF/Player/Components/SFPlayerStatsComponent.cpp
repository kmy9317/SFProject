#include "SFPlayerStatsComponent.h"

#include "Net/UnrealNetwork.h"

USFPlayerStatsComponent::USFPlayerStatsComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

USFPlayerStatsComponent* USFPlayerStatsComponent::FindPlayerStatsComponent(const AActor* Actor)
{
	if (USFPlayerStatsComponent* StatsComponent = Actor ? Actor->FindComponentByClass<USFPlayerStatsComponent>() : nullptr)
	{
		return StatsComponent;
	}

	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (const APlayerState* PS = Pawn->GetPlayerState())
		{
			return PS->FindComponentByClass<USFPlayerStatsComponent>();
		}
	}

	return nullptr;
}

void USFPlayerStatsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, Stats);
}

void USFPlayerStatsComponent::AddDamageDealt(float Amount)
{
	if (GetOwner() && GetOwner()->HasAuthority() && Amount > 0.f)
	{
		Stats.TotalDamageDealt += Amount;
	}
}

void USFPlayerStatsComponent::IncrementDownedCount()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Stats.TotalDownedCount++;
	}
}

void USFPlayerStatsComponent::IncrementReviveCount()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Stats.TotalReviveCount++;
	}
}

void USFPlayerStatsComponent::IncrementKillCount()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Stats.TotalEnemiesKilled++;
	}
}

void USFPlayerStatsComponent::CopyStatsFrom(const USFPlayerStatsComponent* Other)
{
	if (Other)
	{
		Stats = Other->Stats;
	}
}

void USFPlayerStatsComponent::ResetStats()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Stats = FSFPlayerStats();
	}
}

