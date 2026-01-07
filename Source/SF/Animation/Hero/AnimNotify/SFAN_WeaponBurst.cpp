#include "SFAN_WeaponBurst.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayCues/Data/SFDA_WeaponBurstData.h"

USFAN_WeaponBurst::USFAN_WeaponBurst()
{
	NotifyColor = FColor(255, 128, 0, 255); 
}

FString USFAN_WeaponBurst::GetNotifyName_Implementation() const
{
	if (GameplayCueTag.IsValid())
	{
		return FString::Printf(TEXT("Burst Cue: %s"), *GameplayCueTag.ToString());
	}
	return Super::GetNotifyName_Implementation();
}

void USFAN_WeaponBurst::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	if (!GameplayCueTag.IsValid())
	{
		return;
	}

	// 파라미터 구성
	FGameplayCueParameters CueParameters;
	CueParameters.SourceObject = BurstData;    // 우리가 만든 데이터 에셋 전달
	CueParameters.Instigator = OwnerActor;
	CueParameters.Location = OwnerActor->GetActorLocation();

	// [수정] Library 대신 직접 ASC를 찾아 실행 (더 안전하고 명확함)
	// UAbilitySystemGlobals::GetAbilitySystemComponentFromActor는 
	// IAbilitySystemInterface를 구현했거나 컴포넌트로 가진 액터에서 ASC를 안전하게 찾아줍니다.
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
	
	if (ASC)
	{
		// 즉시성(Burst) GameplayCue 실행
		ASC->ExecuteGameplayCue(GameplayCueTag, CueParameters);
	}
}