#include "SFGameInstance.h"

#include "SFInitGameplayTags.h"
#include "Components/GameFrameworkComponentManager.h"

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
		//GetWorld()->ServerTravel(LevelURL.ToString() + "?listen");
		GetWorld()->ServerTravel(LevelURL.ToString(), true);
	}
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
            // 먼저 Base로 읽어서 타입 확인
            FAbilityBaseData* BaseData = EnemyAbilityDataTable->FindRow<FAbilityBaseData>(RowName, TEXT(""));
            
            if (!BaseData )
            {
                continue;
            	
            }
        	
            FAbilityDataWrapper Wrapper;
            
            switch (BaseData->AbilityType)
            {
                case EAbilityType::Attack:
                {
                    FEnemyAttackAbilityData* AttackData = EnemyAbilityDataTable->FindRow<FEnemyAttackAbilityData>(RowName, TEXT(""));
                    if (AttackData)
                    {
                        Wrapper = FAbilityDataWrapper(AttackData, EAbilityType::Attack);
                        EnemyAbilityMap.Add(RowName, Wrapper);
                    }
                    break;
                }
            default:
            	    break;
            }
        }
    	
    }
}
const FAbilityBaseData* USFGameInstance::FindAbilityData(FName AbilityID) const
{
	if (const FAbilityDataWrapper* Wrapper = EnemyAbilityMap.Find(AbilityID))
	{
		return Wrapper->Data;
	}
    
	return nullptr;
}

FAbilityDataWrapper* USFGameInstance::FindAbilityDataWrapper(FName AbilityID)
{
	return EnemyAbilityMap.Find(AbilityID);
}
