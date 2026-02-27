#include "SFBuffArea.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ASFBuffArea::ASFBuffArea()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	AreaASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AreaASC")); //장판용 ASC
	AreaASC->SetIsReplicated(true);

	SourceASC = nullptr;
}

void ASFBuffArea::InitializeArea(UAbilitySystemComponent* InSourceASC)
{
	SourceASC = InSourceASC; //Effect 컨텍스트에 사용
}

void ASFBuffArea::BeginPlay()
{
	Super::BeginPlay();

	//================ GameplayCue 실행 ================
	if (AreaASC && AreaCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = GetActorLocation(); //장판 위치 기준

		//지속형 Cue로 사용 (OnActive/WhileActive 구현되어 있어야 함)
		AreaASC->AddGameplayCue(AreaCueTag, Params);
	}

	//================ Tick 타이머 =====================
	if (TickInterval > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			TickTimerHandle,
			this,
			&ASFBuffArea::OnTickArea,
			TickInterval,
			true
		);
	}

	//================ Duration 타이머 =================
	if (Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			DurationTimerHandle,
			this,
			&ASFBuffArea::OnDurationEnd,
			Duration,
			false
		);
	}
	else
	{
		//Duration <= 0 이면 바로 종료
		OnDurationEnd();
	}
}

//================ 타겟 유효성 검사 =================
bool ASFBuffArea::IsValidTarget(AActor* Target) const
{
	if (!IsValid(Target)) return false;

	if (RequiredActorTag != NAME_None)
	{
		if (!Target->ActorHasTag(RequiredActorTag)) return false;
	}

	return true;
}

//================ TickInterval마다 범위 체크 =================
void ASFBuffArea::OnTickArea()
{
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<AActor*> Candidates;

	// TODO : 전체 Actor를 가져오면 너무 부하가 큼
	UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), Candidates);

	const FVector Origin = GetActorLocation();
	const float RadiusSq = Radius * Radius;

	//이번 Tick에서 범위 안에 있는 대상들
	TSet<TWeakObjectPtr<AActor>> InsideSet;

	for (AActor* Candidate : Candidates)
	{
		if (!IsValidTarget(Candidate)) continue;

		const float DistSq = FVector::DistSquared(
			Origin,
			Candidate->GetActorLocation()
		);

		if (DistSq <= RadiusSq)
		{
			InsideSet.Add(Candidate);
		}
	}

	//1) 범위 안인데 아직 GE 안 걸려 있으면 Apply
	for (const TWeakObjectPtr<AActor>& InsideTarget : InsideSet)
	{
		if (!InsideTarget.IsValid()) continue;

		if (!ActiveEffectMap.Contains(InsideTarget))
		{
			ApplyEffectsTo(InsideTarget.Get());
		}
	}

	//2) 이전에 GE가 걸려 있었는데, 이번 Tick에 범위 밖으로 나간 대상 → Remove
	TArray<TWeakObjectPtr<AActor>> ToRemoveKeys;

	for (auto& Pair : ActiveEffectMap)
	{
		const TWeakObjectPtr<AActor>& TargetPtr = Pair.Key;

		if (!TargetPtr.IsValid() || !InsideSet.Contains(TargetPtr))
		{
			if (TargetPtr.IsValid())
			{
				RemoveEffectsFrom(TargetPtr.Get());
			}
			ToRemoveKeys.Add(TargetPtr);
		}
	}

	for (const TWeakObjectPtr<AActor>& Key : ToRemoveKeys)
	{
		ActiveEffectMap.Remove(Key);
	}
}

//================ GE 적용 =================
void ASFBuffArea::ApplyEffectsTo(AActor* Target)
{
	if (!IsValidTarget(Target)) return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

	if (!TargetASC) return;

	//Effect 컨텍스트는 SourceASC 기준으로 생성 (없으면 TargetASC)
	UAbilitySystemComponent* ContextASC =
		SourceASC ? SourceASC : TargetASC;

	TArray<FActiveGameplayEffectHandle> Handles;

	for (TSubclassOf<UGameplayEffect> EffectClass : EffectsToApply)
	{
		if (!*EffectClass) continue;

		FGameplayEffectContextHandle ContextHandle =
			ContextASC->MakeEffectContext();
		ContextHandle.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle =
			ContextASC->MakeOutgoingSpec(EffectClass, 1.f, ContextHandle);

		if (!SpecHandle.IsValid()) continue;

		FActiveGameplayEffectHandle GEHandle =
			TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

		if (GEHandle.IsValid())
		{
			Handles.Add(GEHandle);
		}
	}

	if (Handles.Num() > 0)
	{
		TWeakObjectPtr<AActor> Key = Target;
		ActiveEffectMap.Add(Key, Handles);
	}
}

//================ GE 제거 =================
void ASFBuffArea::RemoveEffectsFrom(AActor* Target)
{
	if (!IsValid(Target)) return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

	if (!TargetASC) return;

	TWeakObjectPtr<AActor> Key = Target;
	TArray<FActiveGameplayEffectHandle>* HandlesPtr = ActiveEffectMap.Find(Key);

	if (!HandlesPtr) return;

	for (const FActiveGameplayEffectHandle& Handle : *HandlesPtr)
	{
		if (Handle.IsValid())
		{
			TargetASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	HandlesPtr->Empty();
}

//================ Duration 끝났을 때 =================
void ASFBuffArea::OnDurationEnd()
{
	CleanupArea();
	Destroy();
}

//================ 전체 정리 =================
void ASFBuffArea::CleanupArea()
{
	//타이머 정리
	GetWorldTimerManager().ClearTimer(TickTimerHandle);
	GetWorldTimerManager().ClearTimer(DurationTimerHandle);

	//모든 대상에서 GE 제거
	TArray<TWeakObjectPtr<AActor>> Keys;
	ActiveEffectMap.GetKeys(Keys);

	for (const TWeakObjectPtr<AActor>& Key : Keys)
	{
		if (Key.IsValid())
		{
			RemoveEffectsFrom(Key.Get());
		}
	}
	ActiveEffectMap.Empty();

	//GameplayCue 제거
	if (AreaASC && AreaCueTag.IsValid())
	{
		AreaASC->RemoveGameplayCue(AreaCueTag);
	}
}
