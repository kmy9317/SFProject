// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGA_Skill_Melee.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"

void USFGA_Skill_Melee::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void USFGA_Skill_Melee::ParseTargetData(const FGameplayAbilityTargetDataHandle& InTargetDataHandle, TArray<int32>& OutActorsHitIndexes)
{
	
}

void USFGA_Skill_Melee::ProcessHitResult(FHitResult HitResult, float Damage /*, ASFEquipmentBase WeaponActor */)
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
		// const TSubclassOf<UGameplayEffect> DamageGE = USFAssetManager::GetSubclassByPath(USFGameData::Get().DamageGameplayEffect_SetByCaller);
		// FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingGameplayEffectSpec(DamageGE);

		// Effect Context 설정 - 히트 정보와 Causer 엑터 정보 포함
		// FGameplayEffectContextHandle EffectContextHandle = MakeEffectContext(CurrentSpecHandle, CurrentActorInfo);
		// EffectContextHandle.AddHitResult(HitResult);   
		// EffectContextHandle.AddInstigator(SourceASC->AbilityActorInfo->OwnerActor.Get(), WeaponActor);  // Causer 엑터 설정
		// EffectSpecHandle.Data->SetContext(EffectContextHandle);

		// TODO : 데미지 적용전 특정 조건에 따라 BaseDamage 수치 조절 가능
		
		// SetByCaller로 데미지 값 설정 및 GameplayEffect 적용
		// EffectSpecHandle.Data->SetSetByCallerMagnitude(SFGameplayTags::Data_Damage_BaseDamage, Damage);
		// ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, EffectSpecHandle, TargetDataHandle);
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
