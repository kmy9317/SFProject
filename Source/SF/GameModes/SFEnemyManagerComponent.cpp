#include "SFEnemyManagerComponent.h"

#include "SFLogChannels.h"
#include "Net/UnrealNetwork.h"

USFEnemyManagerComponent::USFEnemyManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	AliveEnemyCount = 0;
	TotalEnemyCount = 0;

	bStageCleared = false;
}

void USFEnemyManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, AliveEnemyCount);
	DOREPLIFETIME(ThisClass, TotalEnemyCount);
}

void USFEnemyManagerComponent::RegisterEnemy(ASFEnemy* Enemy)
{
	if (!GetOwner()->HasAuthority() || !Enemy)
	{
		return;
	}

	// 이미 등록된 적인지 확인
	if (RegisteredEnemies.Contains(Enemy))
	{
		return;
	}

	RegisteredEnemies.Add(Enemy);
	AliveEnemyCount++;
	TotalEnemyCount++;

	OnEnemyCountChanged.Broadcast(AliveEnemyCount, TotalEnemyCount);
}

void USFEnemyManagerComponent::UnregisterEnemy(ASFEnemy* Enemy)
{
	if (!GetOwner()->HasAuthority() || !Enemy)
	{
		return;
	}

	// 등록된 적이 아니면 무시
	if (!RegisteredEnemies.Contains(Enemy))
	{
		return;
	}

	RegisteredEnemies.Remove(Enemy);
	AliveEnemyCount = FMath::Max(0, AliveEnemyCount - 1);

	OnEnemyCountChanged.Broadcast(AliveEnemyCount, TotalEnemyCount);

	CheckAllEnemiesDefeated();
}

void USFEnemyManagerComponent::NotifyAllEnemiesSpawned()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	bAllEnemiesSpawned = true;

	CheckAllEnemiesDefeated();
}

void USFEnemyManagerComponent::CheckAllEnemiesDefeated()
{
	// 스폰 완료 전에는 체크하지 않음
	if (!bAllEnemiesSpawned)
	{
		return;
	}
	
	// 아직 적이 남아있으면 대기
	if (AliveEnemyCount > 0)
	{
		return;
	}

	// 이미 스테이지 클리어 했으면 무시
	if (bStageCleared)
	{
		return;
	}

	bStageCleared = true;

	UE_LOG(LogSF, Warning, TEXT("[EnemyManager] All enemies defeated! Activating portal..."));

	// 델리게이트 브로드캐스트
	OnAllEnemiesDefeated.Broadcast();
}

void USFEnemyManagerComponent::OnRep_AliveEnemyCount()
{
	OnEnemyCountChanged.Broadcast(AliveEnemyCount, TotalEnemyCount);
}

void USFEnemyManagerComponent::OnRep_TotalEnemyCount()
{
	OnEnemyCountChanged.Broadcast(AliveEnemyCount, TotalEnemyCount);
}
