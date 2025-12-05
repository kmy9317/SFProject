// SFCombatSlotManager.cpp

#include "SFCombatSlotManager.h"
#include "AI/Controller/SFEnemyController.h"
#include "GameFramework/GameStateBase.h" 
#include "GameFramework/PlayerState.h"   
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

bool USFCombatSlotManager::RequestSlot(ASFEnemyController* Requester, AActor* Target, bool bForce)
{
	if (!Requester || !Target) return false;

	FSFCombatSlotInfo& SlotInfo = TargetSlots.FindOrAdd(Target);

	// 1. 먼저 유효성 검사 및 정리 (죽은 AI 정리 & 현재 인원 파악)
	// (순서 변경: 이미 가지고 있는지 확인하기 전에, 현재 상황(인원수)을 먼저 알아야 '방출' 여부를 결정할 수 있음)
	const int32 CurrentValidSlots = CleanupAndCountSlots(SlotInfo);

	// 전투 시작 시간 기록 (0명일 때)
	if (CurrentValidSlots == 0)
	{
		SlotInfo.CombatStartTime = GetWorld()->GetTimeSeconds();
	}
	
	// 2. 최대 허용 슬롯 계산
	const int32 MaxAllowedSlots = CalculateMaxSlotsForTarget(Target, SlotInfo);

	// 3. 이미 보유 중인 경우 (유지 or 방출 판단)
	if (SlotInfo.SlotHolders.Contains(Requester))
	{
		// 강제 공격권(bForce)이 없는데, 현재 인원이 최대치를 초과했다면? -> 슬롯 반납하고 쫓겨남
		if (!bForce && CurrentValidSlots > MaxAllowedSlots)
		{
			SlotInfo.SlotHolders.Remove(Requester);
			Requester->bHasGuardSlot = false; // 쫓겨났으니 false
			return false;
		}

		// 유지 성공
		if (!Requester->bHasGuardSlot) Requester->bHasGuardSlot = true;
		return true;
	}

	// 4. 신규 진입 (자리 확인)
	// Force가 true면 무조건 통과, 아니면 자리가 있어야 통과
	if (bForce || CurrentValidSlots < MaxAllowedSlots)
	{
		SlotInfo.SlotHolders.Add(Requester);
		Requester->bHasGuardSlot = true;
		return true;
	}

	// 자리 없음
	Requester->bHasGuardSlot = false;
	return false;
}


void USFCombatSlotManager::ReleaseSlot(ASFEnemyController* Releaser, AActor* Target)
{
	if (!Releaser) return;

	if (!Target)
	{
		ReleaseAllSlots(Releaser);
		return;
	}

	if (FSFCombatSlotInfo* SlotInfo = TargetSlots.Find(Target))
	{
		if (SlotInfo->SlotHolders.Remove(Releaser) > 0)
		{
			if (CleanupAndCountSlots(*SlotInfo) == 0)
			{
				TargetSlots.Remove(Target);
			}
		}
	}
	Releaser->bHasGuardSlot = false;
}

void USFCombatSlotManager::ReleaseAllSlots(ASFEnemyController* AIController)
{
	if (!AIController) return;

	for (auto It = TargetSlots.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
			continue;
		}

		FSFCombatSlotInfo& SlotInfo = It.Value();
		if (SlotInfo.SlotHolders.Remove(AIController) > 0)
		{
			if (CleanupAndCountSlots(SlotInfo) == 0)
			{
				It.RemoveCurrent(); 
			}
		}
	}
	AIController->bHasGuardSlot = false;
}

bool USFCombatSlotManager::HasSlot(ASFEnemyController* AIController, AActor* Target) const
{
	if (!AIController || !Target) return false;
	if (const FSFCombatSlotInfo* SlotInfo = TargetSlots.Find(Target))
	{
		return SlotInfo->SlotHolders.Contains(AIController);
	}
	return false;
}

bool USFCombatSlotManager::GetSlotInfo(AActor* Target, int32& OutCurrentSlots, int32& OutMaxSlots) const
{
	if (!Target) return false;
	if (const FSFCombatSlotInfo* SlotInfo = TargetSlots.Find(Target))
	{
		OutCurrentSlots = SlotInfo->SlotHolders.Num();
		OutMaxSlots = CalculateMaxSlotsForTarget(Target, *SlotInfo);
		return true;
	}
	OutCurrentSlots = 0;
	OutMaxSlots = 0;
	return false;
}

int32 USFCombatSlotManager::CalculateMaxSlotsForTarget(AActor* Target, const FSFCombatSlotInfo& SlotInfo) const
{
	
	if (!Target) return 5;

	UWorld* World = GetWorld();
	if (!World) return 5;

	int32 NearbyPlayerCount = 0;
	if (AGameStateBase* GameState = World->GetGameState())
	{
		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (!PS) continue;
			APawn* PlayerPawn = PS->GetPawn();
			if (!PlayerPawn) continue;

			float DistSq = FVector::DistSquared(Target->GetActorLocation(), PlayerPawn->GetActorLocation());
			if (DistSq <= (PlayerGroupDistance * PlayerGroupDistance))
			{
				NearbyPlayerCount++;
			}
		}
	}
	NearbyPlayerCount = FMath::Max(1, NearbyPlayerCount);
	
	int32 BaseSlots = SlotInfo.BaseMaxSlots * NearbyPlayerCount;

	int32 ExtraSlots = 0;
	if (CombatTempoScale > 0.0f)
	{
		double CombatDuration = World->GetTimeSeconds() - SlotInfo.CombatStartTime;
		if (CombatDuration > 0.0f)
		{
			ExtraSlots = FMath::FloorToInt(CombatDuration / CombatTempoScale);
		}
	}

	return BaseSlots + ExtraSlots;
}

int32 USFCombatSlotManager::CleanupAndCountSlots(FSFCombatSlotInfo& SlotInfo)
{
	SlotInfo.SlotHolders.RemoveAll([](const TWeakObjectPtr<ASFEnemyController>& Ptr)
	{
		return !Ptr.IsValid();
	});
	return SlotInfo.SlotHolders.Num();
}