#include "SFCombatLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/SFCharacterBase.h"
#include "Engine/OverlapResult.h"
#include "System/SFAssetManager.h"
#include "System/Data/SFGameData.h"

void USFCombatLibrary::ApplyAreaDamage(const FSFAreaDamageParams& Params)
{
	if (!Params.SourceASC || !Params.SourceActor)
	{
		return;
	}

	UWorld* World = Params.SourceActor->GetWorld();
	if (!World)
	{
		return;
	}

	// 오버랩 수행
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Params.SourceActor);
	for (AActor* Ignored : Params.IgnoreActors)
	{
		if (Ignored)
		{
			QueryParams.AddIgnoredActor(Ignored);
		}
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	bool bHit = World->OverlapMultiByObjectType(Overlaps,Params.Origin,FQuat::Identity,ObjectParams,Params.OverlapShape,QueryParams);
	if (!bHit)
	{
		return;
	}

	// GE 클래스 결정
	TSubclassOf<UGameplayEffect> FinalDamageGE = ResolveDamageGEClass(Params.DamageGEClass);
	if (!FinalDamageGE)
	{
		return;
	}

	// EffectCauser 결정
	AActor* FinalCauser = Params.EffectCauser ? Params.EffectCauser : Params.SourceActor;

	// 대상별 적용
	TSet<AActor*> ProcessedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!TargetActor || ProcessedActors.Contains(TargetActor))
		{
			continue;
		}
		ProcessedActors.Add(TargetActor);

		// 팀 필터링
		if (!ShouldDamageTarget(Params.SourceActor, TargetActor))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (!TargetASC)
		{
			continue;
		}

		// Context 생성
		FGameplayEffectContextHandle Context = Params.SourceASC->MakeEffectContext();
		Context.AddInstigator(Params.SourceActor, FinalCauser);

		// 데미지 GE 적용
		FGameplayEffectSpecHandle DamageSpec = Params.SourceASC->MakeOutgoingSpec(FinalDamageGE, 1.0f, Context);
		if (DamageSpec.IsValid())
		{
			if (Params.DamageSetByCallerTag.IsValid())
			{
				DamageSpec.Data->SetSetByCallerMagnitude(Params.DamageSetByCallerTag, Params.DamageAmount);
			}
			Params.SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
		}

		// 디버프 GE 적용
		if (Params.DebuffGEClass)
		{
			FGameplayEffectSpecHandle DebuffSpec = Params.SourceASC->MakeOutgoingSpec(Params.DebuffGEClass, 1.0f, Context);
			if (DebuffSpec.IsValid())
			{
				Params.SourceASC->ApplyGameplayEffectSpecToTarget(*DebuffSpec.Data.Get(), TargetASC);
			}
		}
	}
}

bool USFCombatLibrary::ShouldDamageTarget(AActor* SourceActor, AActor* TargetActor)
{
	if (!SourceActor || !TargetActor)
	{
		return false;
	}

	ASFCharacterBase* SourceChar = Cast<ASFCharacterBase>(SourceActor);
	ASFCharacterBase* TargetChar = Cast<ASFCharacterBase>(TargetActor);

	if (SourceChar && TargetChar)
	{
		return SourceChar->GetTeamAttitudeTowards(*TargetChar) != ETeamAttitude::Friendly;
	}

	// 캐릭터가 아닌 대상(파괴 가능 오브젝트 등)은 기본 허용
	return true;
}

TSubclassOf<UGameplayEffect> USFCombatLibrary::ResolveDamageGEClass(TSubclassOf<UGameplayEffect> InClass)
{
	if (InClass)
	{
		return InClass;
	}
	return USFAssetManager::GetSubclassByPath(USFGameData::Get().DamageGameplayEffect_SetByCaller);
}