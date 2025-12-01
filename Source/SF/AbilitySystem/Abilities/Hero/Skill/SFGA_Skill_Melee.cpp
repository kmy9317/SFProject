#include "SFGA_Skill_Melee.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterBase.h"
#include "System/SFAssetManager.h"
#include "System/Data/SFGameData.h"
#include "Weapons/Actor/SFEquipmentBase.h"

USFGA_Skill_Melee::USFGA_Skill_Melee(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

void USFGA_Skill_Melee::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ResetHitActors();
}

void USFGA_Skill_Melee::ParseTargetData(const FGameplayAbilityTargetDataHandle& InTargetDataHandle, TArray<int32>& OutActorsHitIndexes)
{
	// 모든 타겟 데이터를 순회
	for (int32 i = 0; i < InTargetDataHandle.Data.Num(); i++)
	{
		const TSharedPtr<FGameplayAbilityTargetData>& TargetData = InTargetDataHandle.Data[i];

		// 히트 결과가 있는지 확인
		if (FHitResult* HitResult = const_cast<FHitResult*>(TargetData->GetHitResult()))
		{
			if (AActor* HitActor = HitResult->GetActor())
			{
				// 히트된 액터가 캐릭터인지 확인
				ASFCharacterBase* TargetCharacter = Cast<ASFCharacterBase>(HitActor);
				if (TargetCharacter == nullptr)
				{
					// 캐릭터가 아니면 Owner를 통해 캐릭터 확인 (예: 부착 장비)
					TargetCharacter = Cast<ASFCharacterBase>(HitActor->GetOwner());
				}

			 // TODO : 적대 관계 확인 - 적대가 아닌 대상은 공격하지 않음 (아군 보호)
			 // if (TargetCharacter && (GetSFCharacterFromActorInfo()->GetTeamAttitudeTowards(*TargetCharacter) != ETeamAttitude::Hostile))
			 // {
			 // 	continue;
			 // }
				
				// 중복 히트 방지를 위해 이미 히트된 액터인지 확인
				AActor* SelectedActor = TargetCharacter ? TargetCharacter : HitActor;
				if (CachedHitActors.Contains(SelectedActor))
				{
					continue;
				}
				
				// 히트된 액터를 캐시에 추가하여 같은 스윕에서 중복 히트 방지
				CachedHitActors.Add(SelectedActor);
				if (TargetCharacter)
				{
					OutActorsHitIndexes.Add(i);
				}
			}
		}
	}
}

void USFGA_Skill_Melee::ProcessHitResult(FHitResult HitResult, float Damage, ASFEquipmentBase* WeaponActor)
{
	USFAbilitySystemComponent* SourceASC = GetSFAbilitySystemComponentFromActorInfo();
	if (SourceASC == nullptr)
	{
		return;
	}

	// 해당 스코프 내 작업은 클라이언트에서 실행하고 서버에서 검증
	FScopedPredictionWindow	ScopedPrediction(SourceASC, GetCurrentActivationInfo().GetActivationPredictionKey());

	// TODO : GameplayCue 실행
	FGameplayCueParameters SourceCueParams;
	SourceCueParams.Location = HitResult.ImpactPoint; // 충돌 위치
	SourceCueParams.Normal = HitResult.ImpactNormal; // 충돌 표면의 노말 벡터
	SourceCueParams.PhysicalMaterial = HitResult.PhysMaterial; // 충돌 표면의 PhysMaterial
	//SourceASC->ExecuteGameplayCue(SFGameplayTags::GameplayCue_Weapon_Impact, SourceCueParams);

	// 서버에서 데미지 처리 수행 
	if (HasAuthority(&CurrentActivationInfo))
	{
		// 타겟 데이터 핸들 생성 - Hit 액터를 타겟으로 설정
		FGameplayAbilityTargetDataHandle TargetDataHandle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(HitResult.GetActor());

		// 데미지 GameplayEffect 가져오기 (SetByCaller 방식으로 데미지 값 전달)
		const TSubclassOf<UGameplayEffect> DamageGE = USFAssetManager::GetSubclassByPath(USFGameData::Get().DamageGameplayEffect_SetByCaller);
		FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingGameplayEffectSpec(DamageGE);

		// Effect Context 설정 - 히트 정보와 Causer 엑터 정보 포함
		FGameplayEffectContextHandle EffectContextHandle = MakeEffectContext(CurrentSpecHandle, CurrentActorInfo);
		EffectContextHandle.AddHitResult(HitResult);   
		EffectContextHandle.AddInstigator(SourceASC->AbilityActorInfo->OwnerActor.Get(), WeaponActor);  // Causer 엑터 설정
		EffectSpecHandle.Data->SetContext(EffectContextHandle);

		// TODO : 데미지 적용전 특정 조건에 따라 SetByCaller Damage 수치 조절 가능
		
		//SetByCaller로 데미지 값 설정 및 GameplayEffect 적용
		EffectSpecHandle.Data->SetSetByCallerMagnitude(SFGameplayTags::Data_Damage_BaseDamage, Damage);
		ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, EffectSpecHandle, TargetDataHandle);
	}

	DrawDebugHitPoint(HitResult);
}

void USFGA_Skill_Melee::DrawDebugHitPoint(const FHitResult& HitResult)
{
	if (bShowDebug)
	{
		// 서버: 시안 / 클라이언트 : 초록색으로 구분하여 네트워크 디버깅 용이
		FColor Color = HasAuthority(&CurrentActivationInfo) ? FColor::Cyan : FColor::Green;
		DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 4, 32, Color, false, 5);
	}
}

void USFGA_Skill_Melee::ResetHitActors()
{
	CachedHitActors.Reset();
}
