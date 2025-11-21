// SFCombatSlotManager.cpp

#include "SFCombatSlotManager.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "AI/Controller/SFEnemyController.h"

bool USFCombatSlotManager::RequestSlot(ASFEnemyController* Requester, AActor* Target)
{
	if (!Requester || !Target)
	{
		UE_LOG(LogTemp, Error, TEXT("[SFCombatSlotManager] RequestSlot: Invalid Requester or Target!"));
		return false;
	}

	// 1. 타겟 슬롯 정보 가져오기 (없으면 생성)
	FSFCombatSlotInfo& SlotInfo = TargetSlots.FindOrAdd(Target);

	// [주의] SFEnemyController에 MaxSlotsPerPlayer 변수가 없다면 추가해야 합니다!
	// 기본값을 3으로 가정하거나, SFEnemyController.h에 변수를 추가하세요.
	// 여기서는 하드코딩된 기본값 3을 사용하지 않고, 컨트롤러 설정을 우선시하는 로직을 유지합니다.
	/*
	if (SlotInfo.MaxSlots == 3 && Requester->MaxSlotsPerPlayer != 3)
	{
		SlotInfo.MaxSlots = Requester->MaxSlotsPerPlayer;
	}
	*/
	// 만약 SFEnemyController에 해당 변수가 없다면 위 코드를 주석 처리하고 아래를 사용하세요.
	// SlotInfo.MaxSlots = 3; 

	// 2. 정리
	CleanupInvalidSlotHolders(SlotInfo);

	// 3. 이미 보유 중인지 확인
	if (SlotInfo.SlotHolders.Contains(Requester))
	{
		return true;
	}

	// 4. 가용성 확인
	const int32 EffectiveMaxSlots = GetEffectiveMaxSlots(Target);
	const int32 CurrentSlots = SlotInfo.SlotHolders.Num();

	if (CurrentSlots >= EffectiveMaxSlots)
	{
		return false;
	}

	// 5. 할당
	SlotInfo.SlotHolders.Add(Requester);
	
	// [변경] bHasAttackSlot -> bHasGuardSlot
	Requester->bHasGuardSlot = true;

	FString PawnName = Requester->GetPawn() ? Requester->GetPawn()->GetName() : TEXT("Unknown");
	UE_LOG(LogTemp, Log, TEXT("[SFCombatSlotManager] ✅ %s: SLOT GRANTED for %s → Slots %d/%d"),
		*PawnName, *Target->GetName(), CurrentSlots + 1, EffectiveMaxSlots);

	return true;
}

void USFCombatSlotManager::ReleaseSlot(ASFEnemyController* Releaser, AActor* Target)
{
	if (!Releaser)
	{
		return;
	}

	FString PawnName = Releaser->GetPawn() ? Releaser->GetPawn()->GetName() : TEXT("Unknown");

	if (!Target)
	{
		ReleaseAllSlots(Releaser);
		return;
	}

	FSFCombatSlotInfo* SlotInfo = TargetSlots.Find(Target);
	if (!SlotInfo)
	{
		return;
	}

	CleanupInvalidSlotHolders(*SlotInfo);

	const int32 RemovedCount = SlotInfo->SlotHolders.Remove(Releaser);
	if (RemovedCount > 0)
	{
		// [변경] bHasAttackSlot -> bHasGuardSlot
		Releaser->bHasGuardSlot = false;

		const int32 RemainingSlots = SlotInfo->SlotHolders.Num();
		UE_LOG(LogTemp, Log, TEXT("[SFCombatSlotManager] 🔓 %s: SLOT RELEASED for %s → Slots %d/%d"),
			*PawnName, *Target->GetName(), RemainingSlots, SlotInfo->MaxSlots);

		if (RemainingSlots == 0)
		{
			TargetSlots.Remove(Target);
		}
	}
}

bool USFCombatSlotManager::GetSlotInfo(AActor* Target, int32& OutCurrentSlots, int32& OutMaxSlots) const
{
	if (!Target)
	{
		return false;
	}

	const FSFCombatSlotInfo* SlotInfo = TargetSlots.Find(Target);
	if (!SlotInfo)
	{
		OutCurrentSlots = 0;
		OutMaxSlots = 3;
		return false;
	}

	OutCurrentSlots = SlotInfo->SlotHolders.Num();
	OutMaxSlots = GetEffectiveMaxSlots(Target);
	return true;
}

bool USFCombatSlotManager::HasSlot(ASFEnemyController* AIController, AActor* Target) const
{
	if (!AIController || !Target)
	{
		return false;
	}

	const FSFCombatSlotInfo* SlotInfo = TargetSlots.Find(Target);
	if (!SlotInfo)
	{
		return false;
	}

	return SlotInfo->SlotHolders.Contains(AIController);
}

void USFCombatSlotManager::ReleaseAllSlots(ASFEnemyController* AIController)
{
	if (!AIController)
	{
		return;
	}

	FString PawnName = AIController->GetPawn() ? AIController->GetPawn()->GetName() : TEXT("Unknown");
	TArray<AActor*> TargetsToRemove;

	for (auto& Pair : TargetSlots)
	{
		AActor* Target = Pair.Key;
		FSFCombatSlotInfo& SlotInfo = Pair.Value;

		CleanupInvalidSlotHolders(SlotInfo);

		const int32 RemovedCount = SlotInfo.SlotHolders.Remove(AIController);
		if (RemovedCount > 0)
		{
			if (SlotInfo.SlotHolders.Num() == 0)
			{
				TargetsToRemove.Add(Target);
			}
		}
	}

	for (AActor* Target : TargetsToRemove)
	{
		TargetSlots.Remove(Target);
	}

	// [변경] bHasAttackSlot -> bHasGuardSlot
	AIController->bHasGuardSlot = false;
	
	UE_LOG(LogTemp, Log, TEXT("[SFCombatSlotManager] 🔓 %s: Force Released All Slots"), *PawnName);
}

TArray<AActor*> USFCombatSlotManager::GetPlayerGroup(AActor* Player) const
{
	TArray<AActor*> Group;
	if (!Player) return Group;

	UWorld* World = GetWorld();
	if (!World) return Group;

	Group.Add(Player);

	for (TActorIterator<APlayerController> It(World); It; ++It)
	{
		APlayerController* PC = *It;
		if (!PC || !PC->GetPawn()) continue;

		AActor* OtherPlayer = PC->GetPawn();
		if (OtherPlayer == Player) continue;

		const float Distance = FVector::Dist(Player->GetActorLocation(), OtherPlayer->GetActorLocation());
		if (Distance <= PlayerGroupDistance)
		{
			Group.Add(OtherPlayer);
		}
	}

	return Group;
}

void USFCombatSlotManager::CleanupInvalidSlotHolders(FSFCombatSlotInfo& SlotInfo)
{
	SlotInfo.SlotHolders.RemoveAll([](const TObjectPtr<ASFEnemyController>& AI)
	{
		return !IsValid(AI) || !IsValid(AI->GetPawn());
	});
}

int32 USFCombatSlotManager::GetEffectiveMaxSlots(AActor* Target) const
{
	if (!Target) return 3;

	TArray<AActor*> PlayerGroup = GetPlayerGroup(Target);
	if (PlayerGroup.Num() > 1)
	{
		const FSFCombatSlotInfo* SlotInfo = TargetSlots.Find(Target);
		const int32 BaseSlots = SlotInfo ? SlotInfo->MaxSlots : 3;
		return BaseSlots * PlayerGroup.Num();
	}

	const FSFCombatSlotInfo* SlotInfo = TargetSlots.Find(Target);
	return SlotInfo ? SlotInfo->MaxSlots : 3;
}