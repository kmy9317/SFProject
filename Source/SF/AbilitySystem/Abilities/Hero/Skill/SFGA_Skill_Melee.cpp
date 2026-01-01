#include "SFGA_Skill_Melee.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "SFLogChannels.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/GameplayEffect/Hero/EffectContext/SFTargetDataTypes.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AbilitySystem/Tasks/Combat/SFAbilityTask_UpdateWarpTarget.h"
#include "Character/SFCharacterBase.h"
#include "Equipment/SFEquipmentDefinition.h"
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

	if (bUseWindupWarp)
	{
		StartWindupWarpTask();
	}
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

				// 적대 관계 확인 - 적대가 아닌 대상은 공격하지 않음 (아군 보호)
				if (TargetCharacter && (GetSFCharacterFromActorInfo()->GetTeamAttitudeTowards(*TargetCharacter) != ETeamAttitude::Hostile))
				{
					continue;
				}
				
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

void USFGA_Skill_Melee::OnTrace(FGameplayEventData Payload)
{
	USFAbilitySystemComponent* SourceASC = GetSFAbilitySystemComponentFromActorInfo();
	if (SourceASC == nullptr)
	{
		return;
	}

	if (!Payload.Instigator)
	{
		return;
	}

	ASFEquipmentBase* WeaponActor = const_cast<ASFEquipmentBase*>(Cast<ASFEquipmentBase>(Payload.Instigator));
	if (!WeaponActor)
	{
		return;
	}
	
	if (SourceASC->FindAbilitySpecFromHandle(CurrentSpecHandle))
	{
		FGameplayAbilityTargetDataHandle LocalTargetDataHandle(MoveTemp(Payload.TargetData));

		TArray<int32> ActorHitIndexes;
		ParseTargetData(LocalTargetDataHandle, ActorHitIndexes);

		for (int32 ActorHitIndex : ActorHitIndexes)
		{
			FHitResult HitResult = *LocalTargetDataHandle.Data[ActorHitIndex]->GetHitResult();
			ProcessHitResult(HitResult, GetScaledBaseDamage() * CurrentDamageMultiplier, WeaponActor);
		}
	}
}

void USFGA_Skill_Melee::StartWindupWarpTask()
{
	FName TargetName;
	float Range;
	float InterpSpeed;
	float MaxAngle;

	if (bUseEquipmentWarpSettings)
	{
		// 무기 DataAsset에서 설정 가져오기
		const FSFWeaponWarpSettings* WarpSettings = GetMainHandWarpSettings();
		if (WarpSettings)
		{
			TargetName = WarpSettings->WarpTargetName;
			Range = WarpSettings->WarpRange;
			InterpSpeed = WarpSettings->RotationInterpSpeed;
			MaxAngle = WarpSettings->MaxWindupTurnAngle;
		}
		else
		{
			// 무기에 WarpSettings가 없으면 기본값 사용
			TargetName = TEXT("AttackTarget");
			Range = 200.f;
			InterpSpeed = 10.f;
			MaxAngle = 90.f;
		}
	}
	else
	{
		// Override 값 사용
		TargetName = WarpTargetNameOverride;
		Range = WarpRangeOverride;
		InterpSpeed = RotationInterpSpeedOverride;
		MaxAngle = MaxWindupTurnAngleOverride;
	}

	ActiveWarpTargetName = TargetName;
	ActiveWarpRange = Range;
	CurrentWindupIndex = 0;

	// 초기 방향 설정
	// TODO : 해당 로직 삭제 할지 고려
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (Character)
	{
		CommittedDirection = Character->GetActorForwardVector();
		CommittedLocation = Character->GetActorLocation() + CommittedDirection * Range;
	}

	// Task 생성 및 시작
	WarpTargetTask = USFAbilityTask_UpdateWarpTarget::CreateTask(this, TargetName, Range, InterpSpeed, MaxAngle);

	// 로컬에서만 Task 실행
	if (IsLocallyControlled() && WarpTargetTask)
	{
		//WarpTargetTask->OnWarpTargetCommitted.AddDynamic(this, &ThisClass::OnWarpTargetCommitted);
		WarpTargetTask->ReadyForActivation();
	}

	// Host Player는 수신 대기 안 함
	// if (HasAuthority(&CurrentActivationInfo) && !IsLocallyControlled())
	// {
	// 	USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo();
	// 	if (ASC)
	// 	{
	// 		FAbilityTargetDataSetDelegate& TargetDataDelegate = ASC->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	// 		ServerWarpDataDelegateHandle = TargetDataDelegate.AddUObject(this, &ThisClass::OnServerWarpDirectionReceived);
	// 		ASC->CallReplicatedTargetDataDelegatesIfSet(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	// 	}
	// }
}

void USFGA_Skill_Melee::CleanupWindupWarpTask()
{
	// 서버 델리게이트 해제 (원격 클라이언트용)
	if (HasAuthority(&CurrentActivationInfo) && !IsLocallyControlled() && ServerWarpDataDelegateHandle.IsValid())
	{
		USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			FAbilityTargetDataSetDelegate& TargetDataDelegate = ASC->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
			TargetDataDelegate.Remove(ServerWarpDataDelegateHandle);
		}
		ServerWarpDataDelegateHandle.Reset();
	}

	// Motion Warping 타겟 제거
	if (ActiveWarpTargetName != NAME_None)
	{
		ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
		if (Character)
		{
			if (UMotionWarpingComponent* MotionWarpingComp = Character->GetMotionWarpingComponent())
			{
				MotionWarpingComp->RemoveWarpTarget(ActiveWarpTargetName);
			}
		}
		ActiveWarpTargetName = NAME_None;
	}

	// Task 정리
	if (WarpTargetTask)
	{
		WarpTargetTask->OnWarpTargetCommitted.RemoveAll(this);
		WarpTargetTask = nullptr;
	}

	CurrentWindupIndex = 0;
}

void USFGA_Skill_Melee::OnWarpTargetCommitted(FVector InCommittedDirection, FVector InCommittedLocation)
{
	CurrentWindupIndex++;
	
	// 로컬 예측: 즉시 상태 저장
	CommittedDirection = InCommittedDirection;
	CommittedLocation = InCommittedLocation;

	if (!IsLocallyControlled())
	{
		return;
	}

	if (HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	// 클라이언트: 서버로 데이터 전송
	USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	FScopedPredictionWindow ScopedPrediction(ASC);
	FSFGameplayAbilityTargetData_WarpDirection* WarpData = new FSFGameplayAbilityTargetData_WarpDirection();
	WarpData->WindupIndex = CurrentWindupIndex;
	WarpData->WarpLocation = InCommittedLocation;
	WarpData->WarpRotation = InCommittedDirection.Rotation();
	FGameplayAbilityTargetDataHandle DataHandle(WarpData);

	// 서버로 전송
	ASC->ServerSetReplicatedTargetData(CurrentSpecHandle,CurrentActivationInfo.GetActivationPredictionKey(), DataHandle, FGameplayTag(), ASC->ScopedPredictionKey);
}

void USFGA_Skill_Melee::OnServerWarpDirectionReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
{
	USFAbilitySystemComponent* ASC = GetSFAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// TargetData 소비
	ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());

	// 데이터 추출
	const FSFGameplayAbilityTargetData_WarpDirection* WarpData = static_cast<const FSFGameplayAbilityTargetData_WarpDirection*>(DataHandle.Get(0));
	if (!WarpData)
	{;
		return;
	}

	// 검증
	// if (!ValidateWarpDirection(WarpData->WarpLocation, WarpData->WarpRotation, WarpData->WindupIndex))
	// {
	// 	return;
	// }

	// 서버에서 Motion Warping 적용
	ApplyWarpTarget(WarpData->WarpLocation, WarpData->WarpRotation);

	// 상태 업데이트
	CommittedDirection = WarpData->WarpRotation.Vector();
	CommittedLocation = WarpData->WarpLocation;
	CurrentWindupIndex = WarpData->WindupIndex;
}

bool USFGA_Skill_Melee::ValidateWarpDirection(const FVector& ClientLocation, const FRotator& ClientRotation, int32 WindupIndex) const
{
	// 1. WindupIndex 검증
	if (WindupIndex != CurrentWindupIndex + 1)
	{
		UE_LOG(LogSF, Warning, TEXT("[SFGA_Skill_Melee] WindupIndex mismatch: expected %d, got %d"), CurrentWindupIndex + 1, WindupIndex);
		return false;
	}

	// 2. 위치 검증
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (Character)
	{
		FVector CharacterLocation = Character->GetActorLocation();
		float MaxExpectedDistance = ActiveWarpRange + 200.f;
		float Distance = FVector::Dist(CharacterLocation, ClientLocation);
		if (Distance > MaxExpectedDistance)
		{
			UE_LOG(LogSF, Warning, TEXT("[SFGA_Skill_Melee] WarpLocation too far: %.1f (max: %.1f)"), Distance, MaxExpectedDistance);
			return false;
		}
	}

	// 3. 급격한 회전 검증
	FVector NewDirection = ClientRotation.Vector().GetSafeNormal2D();
	FVector PrevDirection = CommittedDirection.GetSafeNormal2D();
	if (!PrevDirection.IsNearlyZero())
	{
		float DotProduct = FVector::DotProduct(PrevDirection, NewDirection);
		if (DotProduct < -0.7f) // 약 135도 이상 회전
		{
			UE_LOG(LogSF, Warning, TEXT("[SFGA_Skill_Melee] Direction change too drastic: Dot=%.2f"), DotProduct);
			return false;
		}
	}
	
	return true;
}

void USFGA_Skill_Melee::ApplyWarpTarget(const FVector& Location, const FRotator& Rotation)
{
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	if (!Character)
	{
		return;
	}

	UMotionWarpingComponent* MotionWarpingComp = Character->GetMotionWarpingComponent();
	if (!MotionWarpingComp)
	{
		return;
	}

	MotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(ActiveWarpTargetName, Location, Rotation);
}

float USFGA_Skill_Melee::GetScaledBaseDamage() const
{
	return BaseDamage.GetValueAtLevel(GetAbilityLevel());
}

void USFGA_Skill_Melee::ResetHitActors()
{
	CachedHitActors.Reset();
}

void USFGA_Skill_Melee::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bUseWindupWarp)
	{
		CleanupWindupWarpTask();
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
