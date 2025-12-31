#include "SFCommonUpgradeManagerSubsystem.h"

#include "AbilitySystemComponent.h"
#include "SFCommonLootTable.h"
#include "SFCommonRarityConfig.h"
#include "SFCommonUpgradeChoice.h"
#include "SFCommonUpgradeDefinition.h"
#include "SFCommonUpgradeFragment.h"
#include "SFLogChannels.h"
#include "AbilitySystem/Attributes/Hero/SFCombatSet_Hero.h"
#include "Player/SFPlayerState.h"
#include "System/SFAssetManager.h"

bool USFCommonUpgradeManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    // 서버(Authority) 환경에서만 생성
    UWorld* World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld() && World->GetNetMode() < NM_Client;
}

void USFCommonUpgradeManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // TODO : 플레이어가 가지고 있는 Rerol을 위한 Tag 확정시 재 지정
    // RerollCostTag = TAG_RerollTicket;
    
    // 게임 시작 시 AssetManager가 로드해둔 RarityConfig 캐싱
    CacheCoreData();
}

void USFCommonUpgradeManagerSubsystem::CacheCoreData()
{
    USFAssetManager& AssetManager = USFAssetManager::Get();
    TArray<UObject*> LoadedObjects;
    
    // 이미 메모리에 로드된 RarityConfig 목록을 가져옴
    AssetManager.GetPrimaryAssetObjectList(USFCommonRarityConfig::GetCommonRarityConfigAssetType(), LoadedObjects);

    CachedRarityConfigs.Empty();
    
    for (UObject* Obj : LoadedObjects)
    {
        if (USFCommonRarityConfig* RarityConfig = Cast<USFCommonRarityConfig>(Obj))
        {
            CachedRarityConfigs.Add(RarityConfig);
        }
    }
    
    // BaseWeight 기준 정렬 (낮은 순 = 희귀한 순)
    CachedRarityConfigs.Sort([](const USFCommonRarityConfig& A, const USFCommonRarityConfig& B) 
    {
         return A.BaseWeight < B.BaseWeight; 
    });
}

TArray<FSFCommonUpgradeChoice> USFCommonUpgradeManagerSubsystem::GenerateUpgradeOptions(ASFPlayerState* PlayerState, USFCommonLootTable* LootTable, int32 Count)
{
    TArray<FSFCommonUpgradeChoice> NewChoices;
    if (!PlayerState || !LootTable)
    {
        return NewChoices;
    }

    float LuckValue = GetPlayerLuck(PlayerState);
    TSet<USFCommonUpgradeDefinition*> SelectedDefinitions;

    for (int32 i = 0; i < Count; ++i)
    {
        // Luck 기반 등급 결정
        USFCommonRarityConfig* ChosenRarity = PickRandomRarity(LuckValue);
        FGameplayTag RarityTag = ChosenRarity ? ChosenRarity->RarityTag : FGameplayTag();
        
        // LootTable에서 가중치 랜덤으로 아이템 뽑기(해당 등급에서 허용된 Definition만 선택)
        USFCommonUpgradeDefinition* ChosenDef = PickRandomUpgrade(LootTable, SelectedDefinitions, RarityTag);
        if (!ChosenDef)
        {
            continue;
        }
        
        SelectedDefinitions.Add(ChosenDef);

        // 결과 구조체 생성
        FSFCommonUpgradeChoice Choice;
        Choice.UpgradeDefinition = ChosenDef;
        Choice.RarityConfig = ChosenRarity;
        Choice.UniqueId = FGuid::NewGuid(); // 고유 ID (서버 검증용)
        
        // 수치 계산 (StatBoost Fragment가 있다면 수치를 계산해서 미리 확정)
        if (const USFCommonUpgradeFragment_StatBoost* Fragment = ChosenDef->FindFragment<USFCommonUpgradeFragment_StatBoost>())
        {
            // 등급 태그로 해당 등급의 수치 범위에서 랜덤 선택
            Choice.FinalMagnitude = Fragment->GetRandomMagnitudeForRarity(RarityTag);
            // UI 표시용 텍스트 생성
            FText DisplayValue = Fragment->FormatDisplayValue(Choice.FinalMagnitude);
            Choice.DynamicDescription = FText::Format(ChosenDef->DescriptionFormat, DisplayValue);
        }
        else if (const USFCommonUpgradeFragment_SkillLevel* SkillFragment = ChosenDef->FindFragment<USFCommonUpgradeFragment_SkillLevel>())
        {
            Choice.DynamicDescription = FText::Format(ChosenDef->DescriptionFormat, FText::AsNumber(SkillFragment->LevelIncrement));
        }
        else
        {
            Choice.DynamicDescription = ChosenDef->DescriptionFormat;
        }

        NewChoices.Add(Choice);
    }

    // 컨텍스트 저장
    FSFCommonUpgradeContext& Context = ActiveUpgradeContexts.FindOrAdd(PlayerState);
    Context.SourceLootTable = LootTable;
    Context.SlotCount = Count;
    Context.PendingChoices = NewChoices;

    return NewChoices;
}

float USFCommonUpgradeManagerSubsystem::GetPlayerLuck(ASFPlayerState* PlayerState) const
{
    if (!PlayerState)
    {
        return 0.0f;
    }

    UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
    if (!ASC)
    {
        return 0.0f;
    }

    bool bFound = false;
    float LuckValue = ASC->GetGameplayAttributeValue(USFCombatSet_Hero::GetLuckAttribute(), bFound);

    return bFound ? LuckValue : 0.0f;
}


TArray<FSFCommonUpgradeChoice> USFCommonUpgradeManagerSubsystem::TryRerollOptions(ASFPlayerState* PlayerState)
{
    TArray<FSFCommonUpgradeChoice> EmptyResult;
    
    if (!PlayerState)
    {
        return EmptyResult;
    }
    
    UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
    if (!ASC)
    {
        return EmptyResult;
    }
    
    // 리롤 티켓 확인 (실제로는 PlayerCombatState의 bool값을 체크할 수도 있음)
    int32 TicketCount = ASC->GetGameplayTagCount(RerollCostTag);
    if (TicketCount <= 0)
    {
        return EmptyResult; // 티켓 부족 -> 리롤 실패
    }

    const FSFCommonUpgradeContext* ContextPtr = ActiveUpgradeContexts.Find(PlayerState);
    if (!ContextPtr || !ContextPtr->SourceLootTable)
    {
        return EmptyResult; // 이 플레이어는 현재 리롤할 수 있는 선택지가 없음
    }

    // GenerateUpgradeOptions 내부에서 Map을 갱신할 수 있으므로 필요한 정보를 미리 복사
    USFCommonLootTable* SourceTable = ContextPtr->SourceLootTable;
    int32 SlotCount = ContextPtr->SlotCount;

    // 비용 소모(TODO : 추후 PlayerCombatState의 bool등을 설정할 수 있음)
    // ASC->RemoveLooseGameplayTag(RerollCostTag, 1);

    // 찾아낸 테이블과 개수 정보를 사용하여 리롤
    return GenerateUpgradeOptions(PlayerState, SourceTable, SlotCount);
}

bool USFCommonUpgradeManagerSubsystem::ApplyUpgradeChoice(ASFPlayerState* PlayerState, const FGuid& ChoiceId)
{
    if (!PlayerState || !ChoiceId.IsValid())
    {
        return false;
    }

    FSFCommonUpgradeContext* Context = ActiveUpgradeContexts.Find(PlayerState);
    if (!Context)
    {
        return false;
    }

    // PendingChoices에서 해당 ID 찾기
    const FSFCommonUpgradeChoice* FoundChoice = nullptr;
    for (const FSFCommonUpgradeChoice& Choice : Context->PendingChoices)
    {
        if (Choice.UniqueId == ChoiceId)
        {
            FoundChoice = &Choice;
            break;
        }
    }
    
    if (!FoundChoice)
    {
        return false;
    }
    if (!FoundChoice->UpgradeDefinition)
    {
        return false;
    }

    UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
    if (!ASC)
    {
        return false;
    }

    // Fragment별 효과 적용
    for (const USFCommonUpgradeFragment* Fragment : FoundChoice->UpgradeDefinition->Fragments)
    {
        if (!Fragment)
        {
            continue;
        }

        if (const auto* StatBoost = Cast<USFCommonUpgradeFragment_StatBoost>(Fragment))
        {
            ApplyStatBoostFragment(ASC, StatBoost, FoundChoice->FinalMagnitude);
        }
        if (const auto* SkillLevel = Cast<USFCommonUpgradeFragment_SkillLevel>(Fragment))
        {
            ApplySkillLevelFragment(ASC, SkillLevel);
        }
    }

    // 컨텍스트 정리
    ActiveUpgradeContexts.Remove(PlayerState);

    return true;
}

bool USFCommonUpgradeManagerSubsystem::ApplyUpgradeChoiceByIndex(ASFPlayerState* PlayerState, int32 ChoiceIndex)
{
    if (!PlayerState)
    {
        return false;
    }

    FSFCommonUpgradeContext* Context = ActiveUpgradeContexts.Find(PlayerState);
    if (!Context)
    {
        return false;
    }

    if (!Context->PendingChoices.IsValidIndex(ChoiceIndex))
    {
        return false;
    }

    return ApplyUpgradeChoice(PlayerState, Context->PendingChoices[ChoiceIndex].UniqueId);
}

USFCommonUpgradeDefinition* USFCommonUpgradeManagerSubsystem::PickRandomUpgrade(const USFCommonLootTable* Table, const TSet<USFCommonUpgradeDefinition*>& ExcludedItems, const FGameplayTag& RarityTag)
{
    if (!Table)
    {
        return nullptr;
    }

    // 유효한 후보 찾기
    TArray<TPair<USFCommonUpgradeDefinition*, float>> ValidCandidates;
    float TotalWeight = 0.0f;

    for (const FSFCommonLootEntry& Entry : Table->LootEntries)
    {
        USFCommonUpgradeDefinition* Def = Entry.UpgradeDefinition.Get();

        if (!Def)
        {
            continue;
        }

        if (ExcludedItems.Contains(Def))
        {
            continue;
        }

        if (!Def->IsAllowedForRarity(RarityTag))
        {
            continue;
        }

        ValidCandidates.Add(TPair<USFCommonUpgradeDefinition*, float>(Def, Entry.Weight));
        TotalWeight += Entry.Weight;
    }

    if (ValidCandidates.IsEmpty() || TotalWeight <= 0.0f)
    {
        return nullptr;
    }

    // 유효한 후보에서 랜덤 선택
    float RandomPoint = FMath::FRandRange(0.0f, TotalWeight);
    for (const auto& Candidate : ValidCandidates)
    {
        RandomPoint -= Candidate.Value;
        if (RandomPoint <= 0.0f)
        {
            return Candidate.Key;
        }
    }

    return ValidCandidates.Last().Key;
}

USFCommonRarityConfig* USFCommonUpgradeManagerSubsystem::PickRandomRarity(float LuckValue)
{
    if (CachedRarityConfigs.IsEmpty())
    {
        return nullptr;
    }

    // Luck 기반 가중치 계산
    float TotalWeight = 0.0f;
    TArray<TPair<USFCommonRarityConfig*, float>> WeightedConfigs;
    for (USFCommonRarityConfig* Config : CachedRarityConfigs)
    {
        float Weight = Config->GetWeightForLuck(LuckValue);
        if (Weight > 0.0f)
        {
            WeightedConfigs.Add(TPair<USFCommonRarityConfig*, float>(Config, Weight));
            TotalWeight += Weight;
        }
    }

    if (TotalWeight <= 0.0f || WeightedConfigs.IsEmpty())
    {
        return CachedRarityConfigs[0];
    }

    // 가중치 랜덤 선택
    float RandomPoint = FMath::FRandRange(0.0f, TotalWeight);
    for (const auto& Pair : WeightedConfigs)
    {
        RandomPoint -= Pair.Value;
        if (RandomPoint <= 0.0f)
        {
            return Pair.Key;
        }
    }

    return WeightedConfigs.Last().Key;
}

void USFCommonUpgradeManagerSubsystem::ApplyStatBoostFragment(UAbilitySystemComponent* ASC, const USFCommonUpgradeFragment_StatBoost* Fragment, float FinalMagnitude)
{
    if (!ASC || !Fragment || !Fragment->EffectClass)
    {
        return;
    }

    FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(Fragment->EffectClass, 1, ContextHandle);

    if (SpecHandle.IsValid())
    {
        SpecHandle.Data->SetSetByCallerMagnitude(Fragment->AttributeTag, FinalMagnitude);
        ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
        UE_LOG(LogSF, Warning, TEXT("Applied StatBoost: Tag=%s, Magnitude=%.2f"), *Fragment->AttributeTag.ToString(), FinalMagnitude);
    }
}

void USFCommonUpgradeManagerSubsystem::ApplySkillLevelFragment(UAbilitySystemComponent* ASC, const USFCommonUpgradeFragment_SkillLevel* Fragment)
{
    if (!ASC || !Fragment || !Fragment->TargetSkillInputTag.IsValid())
    {
        return;
    }

    FScopedAbilityListLock ActiveScopeLock(*ASC);
    for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        if (Spec.GetDynamicSpecSourceTags().HasTagExact(Fragment->TargetSkillInputTag))
        {
            Spec.Level += Fragment->LevelIncrement;
            ASC->MarkAbilitySpecDirty(Spec);

            UE_LOG(LogTemp, Log, TEXT("Applied SkillLevel: Tag=%s, NewLevel=%d"), *Fragment->TargetSkillInputTag.ToString(), Spec.Level);
            break;
        }
    }
}