#include "SFCommonUpgradeManagerSubsystem.h"

#include "AbilitySystemComponent.h"
#include "SFCommonLootTable.h"
#include "SFCommonRarityConfig.h"
#include "SFCommonUpgradeChoice.h"
#include "SFCommonUpgradeDefinition.h"
#include "SFCommonUpgradeFragment.h"
#include "SFLogChannels.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFHeroSkillTags.h"
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

FGuid USFCommonUpgradeManagerSubsystem::GenerateUpgradeOptions(ASFPlayerState* PlayerState, USFCommonLootTable* LootTable, int32 Count, FOnUpgradeComplete OnComplete, AActor* SourceInteractable, TArray<FSFCommonUpgradeChoice>& OutChoices)
{
    if (!PlayerState || !LootTable)
    {
        return FGuid();
    }

    // 같은 플레이어 + 같은 상자의 기존 컨텍스트 검색
    for (auto& Pair : ActiveUpgradeContexts)
    {
        FSFCommonUpgradeContext& Existing = Pair.Value;
        if (Existing.OwnerPlayerState == PlayerState
            && SourceInteractable
            && Existing.SourceInteractable == SourceInteractable
            && !Existing.PendingChoices.IsEmpty())
        {
            Existing.OnCompleteCallback = MoveTemp(OnComplete);
            OutChoices = Existing.PendingChoices;
            return Existing.ContextId;
        }
    }

    FGuid NewContextId = FGuid::NewGuid();
    FSFCommonUpgradeContext& Context = ActiveUpgradeContexts.Add(NewContextId);
    Context.ContextId = NewContextId;
    Context.OwnerPlayerState = PlayerState;
    Context.SourceLootTable = LootTable;
    Context.SourceInteractable = SourceInteractable;
    Context.SlotCount = Count;
    Context.OnCompleteCallback = MoveTemp(OnComplete);
    Context.RerollCount = 0;
    Context.bUsedFreeReroll = false;
    Context.bUsedMoreEnhance = false;

    Context.PendingChoices = CreateNewChoices(PlayerState, LootTable, Count);
    OutChoices = Context.PendingChoices;
    return NewContextId;
}

TArray<FSFCommonUpgradeChoice> USFCommonUpgradeManagerSubsystem::RegenerateChoicesInternal(const FGuid& ContextId)
{
    FSFCommonUpgradeContext* Context = ActiveUpgradeContexts.Find(ContextId);
    if (!Context || !Context->SourceLootTable || !Context->OwnerPlayerState.IsValid())
    {
        return TArray<FSFCommonUpgradeChoice>();
    }

    Context->PendingChoices = CreateNewChoices(Context->OwnerPlayerState.Get(), Context->SourceLootTable, Context->SlotCount);
    return Context->PendingChoices;
}

TArray<FSFCommonUpgradeChoice> USFCommonUpgradeManagerSubsystem::CreateNewChoices(ASFPlayerState* PlayerState, USFCommonLootTable* LootTable, int32 Count)
{
    TArray<FSFCommonUpgradeChoice> NewChoices;
    
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


TArray<FSFCommonUpgradeChoice> USFCommonUpgradeManagerSubsystem::TryRerollOptions(const FGuid& ContextId)
{
    TArray<FSFCommonUpgradeChoice> EmptyResult;
    
    FSFCommonUpgradeContext* Context = ActiveUpgradeContexts.Find(ContextId);
    if (!Context || !Context->SourceLootTable)
    {
        return EmptyResult;
    }

    ASFPlayerState* PlayerState = Context->OwnerPlayerState.Get();
    if (!PlayerState)
    {
        return EmptyResult;
    }

    UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
    if (!ASC)
    {
        return EmptyResult;
    }

    int32 Cost = CalculateRerollCost(ContextId);
    if (Cost == 0 && !Context->bUsedFreeReroll)
    {
        Context->bUsedFreeReroll = true;
    }
    else
    {
        if (PlayerState->GetGold() < Cost)
        {
            return EmptyResult;
        }
        PlayerState->AddGold(-Cost);
        Context->RerollCount++;
    }

    return RegenerateChoicesInternal(ContextId);
}

TArray<FSFCommonUpgradeChoice> USFCommonUpgradeManagerSubsystem::RegenerateChoicesForMoreEnhance(const FGuid& ContextId)
{
    return RegenerateChoicesInternal(ContextId);
}

ESFUpgradeApplyResult USFCommonUpgradeManagerSubsystem::ApplyUpgradeChoice(const FGuid& ContextId, const FGuid& ChoiceId)
{
    if (!ContextId.IsValid() || !ChoiceId.IsValid())
    {
        return ESFUpgradeApplyResult::Failed;
    }

    FSFCommonUpgradeContext* Context = ActiveUpgradeContexts.Find(ContextId);
    if (!Context)
    {
        return ESFUpgradeApplyResult::Failed;
    }

    ASFPlayerState* PlayerState = Context->OwnerPlayerState.Get();
    if (!PlayerState)
    {
        return ESFUpgradeApplyResult::Failed;
    }

    const FSFCommonUpgradeChoice* FoundChoice = nullptr;
    for (const FSFCommonUpgradeChoice& Choice : Context->PendingChoices)
    {
        if (Choice.UniqueId == ChoiceId)
        {
            FoundChoice = &Choice;
            break;
        }
    }

    if (!FoundChoice || !FoundChoice->UpgradeDefinition)
    {
        return ESFUpgradeApplyResult::Failed;
    }

    UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
    if (!ASC)
    {
        return ESFUpgradeApplyResult::Failed;
    }

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

    if (!Context->bUsedMoreEnhance && ASC->HasMatchingGameplayTag(SFGameplayTags::Ability_Skill_Passive_MoreEnhance))
    {
        Context->bUsedMoreEnhance = true;
        const USFGameData& GameData = USFGameData::Get();
        if (FMath::FRand() <= GameData.MoreEnhanceChance)
        {
            return ESFUpgradeApplyResult::MoreEnhance;
        }
    }

    if (Context->OnCompleteCallback.IsBound())
    {
        Context->OnCompleteCallback.Execute();
    }

    ActiveUpgradeContexts.Remove(ContextId);
    return ESFUpgradeApplyResult::Success;
}

ESFUpgradeApplyResult USFCommonUpgradeManagerSubsystem::ApplyUpgradeChoiceByIndex(const FGuid& ContextId, int32 ChoiceIndex)
{
    FSFCommonUpgradeContext* Context = ActiveUpgradeContexts.Find(ContextId);
    if (!Context || !Context->PendingChoices.IsValidIndex(ChoiceIndex))
    {
        return ESFUpgradeApplyResult::Failed;
    }

    return ApplyUpgradeChoice(ContextId, Context->PendingChoices[ChoiceIndex].UniqueId);
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

bool USFCommonUpgradeManagerSubsystem::CanReroll(const FGuid& ContextId) const
{
    const FSFCommonUpgradeContext* Context = ActiveUpgradeContexts.Find(ContextId);
    if (!Context || Context->PendingChoices.IsEmpty())
    {
        return false;
    }

    ASFPlayerState* PlayerState = Context->OwnerPlayerState.Get();
    if (!PlayerState)
    {
        return false;
    }

    int32 Cost = CalculateRerollCost(ContextId);
    return (Cost == 0) || (PlayerState->GetGold() >= Cost);
}

int32 USFCommonUpgradeManagerSubsystem::CalculateRerollCost(const FGuid& ContextId) const
{
    const FSFCommonUpgradeContext* Context = ActiveUpgradeContexts.Find(ContextId);
    if (!Context)
    {
        return GetRerollCostByCount(0);
    }

    if (!Context->bUsedFreeReroll)
    {
        ASFPlayerState* PlayerState = Context->OwnerPlayerState.Get();
        if (PlayerState)
        {
            UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
            if (ASC && ASC->HasMatchingGameplayTag(SFGameplayTags::Ability_Skill_Passive_FreeReroll))
            {
                return 0;
            }
        }
    }

    return GetRerollCostByCount(Context->RerollCount);
}


bool USFCommonUpgradeManagerSubsystem::HasMoreEnhanceAvailable(const FGuid& ContextId) const
{
    const FSFCommonUpgradeContext* Context = ActiveUpgradeContexts.Find(ContextId);
    if (!Context || Context->bUsedMoreEnhance)
    {
        return false;
    }

    ASFPlayerState* PlayerState = Context->OwnerPlayerState.Get();
    if (!PlayerState)
    {
        return false;
    }

    UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
    return ASC && ASC->HasMatchingGameplayTag(SFGameplayTags::Ability_Skill_Passive_MoreEnhance);
}

void USFCommonUpgradeManagerSubsystem::ClearUpgradeContext(const FGuid& ContextId)
{
    ActiveUpgradeContexts.Remove(ContextId);
}

int32 USFCommonUpgradeManagerSubsystem::GetRerollCostByCount(int32 InRerollCount) const
{
    if (InRerollCount <= 0)
    {
        return 20;
    }

    int32 Cost = 20;
    int32 Increment = 10;

    for (int32 i = 0; i < InRerollCount; ++i)
    {
        Cost += Increment;
        Increment *= 2;
    }

    return Cost;
}
