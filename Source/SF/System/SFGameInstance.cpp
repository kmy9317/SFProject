#include "SFGameInstance.h"

#include "GenericTeamAgentInterface.h"
#include "SFInitGameplayTags.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Team/SFTeamTypes.h"

void USFGameInstance::Init()
{
	Super::Init();

	// InitState를 GFCM에 등록
	UGameFrameworkComponentManager* ComponentManager = GetSubsystem<UGameFrameworkComponentManager>(this);

	if (ensure(ComponentManager))
	{
		ComponentManager->RegisterInitState(SFGameplayTags::InitState_Spawned, false, FGameplayTag());
		ComponentManager->RegisterInitState(SFGameplayTags::InitState_DataAvailable, false, SFGameplayTags::InitState_Spawned);
		ComponentManager->RegisterInitState(SFGameplayTags::InitState_DataInitialized, false, SFGameplayTags::InitState_DataAvailable);
		ComponentManager->RegisterInitState(SFGameplayTags::InitState_GameplayReady, false, SFGameplayTags::InitState_DataInitialized);
	}

	InitTeamAttitudeSolver();
	LoadEnemyDataTable();
}

void USFGameInstance::StartMatch()
{
	if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer || GetWorld()->GetNetMode() == ENetMode::NM_ListenServer)
	{
		LoadLevelAndListen(GameLevel);
	}
}

void USFGameInstance::LoadLevelAndListen(TSoftObjectPtr<UWorld> Level)
{
	const FName LevelURL = FName(*FPackageName::ObjectPathToPackageName(Level.ToString()));

	if (LevelURL != "")
	{
		GetWorld()->ServerTravel(LevelURL.ToString() + "?listen", false);
		//GetWorld()->ServerTravel(LevelURL.ToString(), true);
	}
}

void USFGameInstance::InitTeamAttitudeSolver()
{
	// Player vs Enemy = Hostile
	FGenericTeamId::SetAttitudeSolver([](FGenericTeamId A, FGenericTeamId B) -> ETeamAttitude::Type
	{
		if (A == B)
		{
			return ETeamAttitude::Friendly;
		}
		
		// Player(0) vs Enemy(1) = Hostile
		if ((A == FGenericTeamId(SFTeamID::Player) && B == FGenericTeamId(SFTeamID::Enemy)) ||
			(A == FGenericTeamId(SFTeamID::Enemy) && B == FGenericTeamId(SFTeamID::Player)))
		{
			return ETeamAttitude::Hostile;
		}
        
		return ETeamAttitude::Neutral;
	});
}

void USFGameInstance::LoadEnemyDataTable()
{
    // 1. Enemy Attribute 데이터 로드
    if (EnemyDataTable)
    {
        EnemyDataMap.Empty();
        
        TArray<FName> RowNames = EnemyDataTable->GetRowNames();
        for (const FName& RowName : RowNames)
        {
            if (FEnemyAttributeData* RowData = EnemyDataTable->FindRow<FEnemyAttributeData>(RowName, TEXT("")))
            {
                EnemyDataMap.Add(RowName, *RowData);
            }
        }
        
     
    }

    // 2. Enemy Ability 데이터 로드 
    if (EnemyAbilityDataTable)
    {
        EnemyAbilityMap.Empty();

        TArray<FName> RowNames = EnemyAbilityDataTable->GetRowNames();

        for (const FName& RowName : RowNames)
        {
            // Attack 타입으로 먼저 시도 
            if (FEnemyAttackAbilityData* AttackData = EnemyAbilityDataTable->FindRow<FEnemyAttackAbilityData>(RowName, TEXT("")))
            {
                // AbilityType이 Attack인지 확인
                if (AttackData->AbilityType == EAbilityType::Attack)
                {
                    FAbilityDataWrapper Wrapper(*AttackData);
                    EnemyAbilityMap.Add(RowName, Wrapper);
                    continue;
                }
            }

            // 추후 다른 타입 추가 시 여기에 추가
            // if (FEnemyDefensiveAbilityData* DefensiveData = ...)
            // {
            //     if (DefensiveData->AbilityType == EAbilityType::Defensive)
            //     {
            //         FAbilityDataWrapper Wrapper(*DefensiveData);
            //         EnemyAbilityMap.Add(RowName, Wrapper);
            //         continue;
            //     }
            // }
        }
    }
}
const FAbilityBaseData* USFGameInstance::FindAbilityData(FName AbilityID) const
{
	if (const FAbilityDataWrapper* Wrapper = EnemyAbilityMap.Find(AbilityID))
	{
		return Wrapper->GetBaseData();
	}

	return nullptr;
}

FAbilityDataWrapper* USFGameInstance::FindAbilityDataWrapper(FName AbilityID)
{
	return EnemyAbilityMap.Find(AbilityID);
}
