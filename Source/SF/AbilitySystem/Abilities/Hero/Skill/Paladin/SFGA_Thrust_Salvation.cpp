#include "SFGA_Thrust_Salvation.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/GameplayCues/Data/SFDA_WaveEffectData.h"
#include "AbilitySystem/GameplayCues/Data/SFDA_BuffAuraEffectData.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterBase.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Libraries/SFCollisionLibrary.h"
#include "Libraries/SFDrawShapeLibrary.h"
#include "System/SFAssetManager.h"

USFGA_Thrust_Salvation::USFGA_Thrust_Salvation(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_SuperArmor);
	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Skill);
}

void USFGA_Thrust_Salvation::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ASFCharacterBase* SFCharacter = GetSFCharacterFromActorInfo();

	// 공중에서 어빌리티 실행 불가
	if (!SFCharacter || SFCharacter->GetCharacterMovement()->IsFalling())
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}

	if (UAbilityTask_WaitGameplayEvent* ShieldBashEffectBeginTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SFGameplayTags::GameplayEvent_Montage_Begin, nullptr, true, true))
	{
		ShieldBashEffectBeginTask->EventReceived.AddDynamic(this, &ThisClass::OnShieldBashEffectBegin);
		ShieldBashEffectBeginTask->ReadyForActivation();
	}

	// 돌진 몽타주 시작
	if (ThrustMontage)
	{
		if (UAbilityTask_PlayMontageAndWait* ThrustTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("ThrustMontage"), ThrustMontage, 1.f, NAME_None, false))
		{
			ThrustTask->OnCompleted.AddDynamic(this, &ThisClass::OnThrustMontageCompleted);
			ThrustTask->OnBlendOut.AddDynamic(this, &ThisClass::OnThrustMontageCompleted);
			ThrustTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
			ThrustTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageFinished);
			ThrustTask->ReadyForActivation();
		}
	}
	// 돌진 몽타주가 없으면 바로 ShieldBash로
	else if (ShieldBashMontage)
	{
		OnThrustMontageCompleted();
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}


void USFGA_Thrust_Salvation::OnThrustMontageCompleted()
{
	if (!ShieldBashMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}
	
	if (UAbilityTask_PlayMontageAndWait* ShieldBashTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, TEXT("ShieldBashMontage"), ShieldBashMontage, 1.f, NAME_None, true))
	{
		ShieldBashTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
		ShieldBashTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
		ShieldBashTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
		ShieldBashTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageFinished);
		ShieldBashTask->ReadyForActivation();

		SetCameraMode(CameraModeClass);
	}
}

void USFGA_Thrust_Salvation::OnShieldBashEffectBegin(FGameplayEventData Payload)
{
	ASFCharacterBase* SourceCharacter = GetSFCharacterFromActorInfo();
	if (!SourceCharacter)
	{
		return;
	}
	
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}

	// 공격 범위 계산(공격 중심 위치: 캐릭터 위치에서 전방으로 Distance만큼 이동한 위치)
	FVector CapsulePosition = SourceCharacter->GetActorLocation() + (SourceCharacter->GetActorForwardVector() * AttackDistance);

	// 공격 범위 반경: 캐릭터 캡슐 반경에 RadiusMultiplier를 곱한 값
	float Radius = SourceCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() * AttackRadiusMultiplier;

	// 공격 범위 높이: 캐릭터 캡슐 높이와 동일
	float HalfHeight = SourceCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

#if WITH_EDITOR
	if (bShowDebug)
	{
		DrawDebugCapsule(GetWorld(), CapsulePosition, HalfHeight, Radius, 
			FQuat::Identity, FColor::Yellow, false, 2.0f);
	}
#endif

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypeQueries = { UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn) };
	TArray<AActor*> ActorsToIgnore = { SourceCharacter };
	TArray<AActor*> OverlappedActors;

	UKismetSystemLibrary::CapsuleOverlapActors(this, CapsulePosition, Radius, HalfHeight, ObjectTypeQueries, ASFCharacterBase::StaticClass(), ActorsToIgnore, OverlappedActors);
	
	// 각 타겟에게 데미지 및 넉백 효과 적용
	for (AActor* OverlappedActor : OverlappedActors)
	{
		ASFCharacterBase* TargetCharacter = Cast<ASFCharacterBase>(OverlappedActor);
		if (!TargetCharacter)
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetCharacter);
		if (!TargetASC)
		{
			continue;
		}

		const ETeamAttitude::Type Attitude = SourceCharacter->GetTeamAttitudeTowards(*TargetCharacter);

		if (Attitude == ETeamAttitude::Hostile)
		{
			ApplyDamageAndKnockback(SourceCharacter, TargetCharacter, TargetASC);
		}
	}

	ApplyWaveEffectAndAllyBuff();
}

void USFGA_Thrust_Salvation::ApplyDamageAndKnockback(ASFCharacterBase* Source, ASFCharacterBase* Target, UAbilitySystemComponent* TargetASC)
{
	// 데미지 적용
	TSubclassOf<UGameplayEffect> DamageGE = USFAssetManager::GetSubclassByPath(USFGameData::Get().DamageGameplayEffect_SetByCaller);
	FGameplayEffectSpecHandle DamageSpec = MakeOutgoingGameplayEffectSpec(DamageGE);
	DamageSpec.Data->SetSetByCallerMagnitude(SFGameplayTags::Data_Damage_BaseDamage, GetScaledBaseDamage());

	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
	FHitResult HitResult;
	HitResult.ImpactPoint = Target->GetActorLocation();
	HitResult.ImpactNormal = (Target->GetActorLocation() - Source->GetActorLocation()).GetSafeNormal();
	ContextHandle.AddHitResult(HitResult);
	DamageSpec.Data->SetContext(ContextHandle);

	TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());

	// 넉백 이벤트
	FGameplayEventData KnockbackPayload;
	KnockbackPayload.Instigator = Source;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, SFGameplayTags::GameplayEvent_Knockback, KnockbackPayload);
}

void USFGA_Thrust_Salvation::ApplyWaveEffectAndAllyBuff()
{
	ASFCharacterBase* SourceCharacter = GetSFCharacterFromActorInfo();
	if (!SourceCharacter)
	{
		return;
	}

	USFAbilitySystemComponent* SourceASC = GetSFAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}

	const FVector SourceLocation = SourceCharacter->GetActorLocation();
	const FVector ForwardDir = SourceCharacter->GetActorForwardVector();
	
	// 파장 VFX + 사운드 재생
	if (WaveEffectCueTag.IsValid() && WaveEffectData)
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = SourceCharacter->GetActorLocation();
		CueParams.Normal = FVector::UpVector;
		CueParams.Instigator = SourceCharacter;
		CueParams.RawMagnitude = WaveForwardRadius;
		CueParams.SourceObject = WaveEffectData;

		SourceASC->ExecuteGameplayCue(WaveEffectCueTag, CueParams);
	}

	// 아군 버프 (서버에서만)
	if (!HasAuthority(&CurrentActivationInfo) || !BuffEffectClass)
	{
		return;
	}

	// 자기 자신 버프
	if (bBuffSelf)
	{
		ApplyBuffToAlly(SourceASC, SourceASC);
	}
	
	// 타원형 오버랩
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	USFCollisionLibrary::EllipseOverlapActors(
		this,
		SourceLocation,
		ForwardDir,
		WaveForwardRadius,
		WaveSideRadius,
		WaveHalfHeight,
		ObjectTypes,
		ASFCharacterBase::StaticClass(),
		TArray<AActor*>{ SourceCharacter },
		OverlappedActors
	);

#if WITH_EDITOR
	if (bShowDebug)
	{
		// 타원 디버그 드로우
		USFDrawShapeLibrary::DrawDebugEllipse(
		this,
		SourceLocation,
		ForwardDir,
		SourceCharacter->GetActorRightVector(),
		WaveForwardRadius,
		WaveSideRadius,
		WaveHalfHeight, 
		FColor::Green,
		2.0f,
		2.0f);
	}
#endif

	for (AActor* Actor : OverlappedActors)
	{
		ASFCharacterBase* TargetCharacter = Cast<ASFCharacterBase>(Actor);
		if (!TargetCharacter)
		{
			continue;
		}

		// 아군인지 확인
		ETeamAttitude::Type Attitude = SourceCharacter->GetTeamAttitudeTowards(*TargetCharacter);
		if (Attitude != ETeamAttitude::Friendly)
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetCharacter);
		if (TargetASC)
		{
			ApplyBuffToAlly(SourceASC, TargetASC);
		}
	}
}

void USFGA_Thrust_Salvation::ApplyBuffToAlly(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC)
{
	if (!BuffEffectClass || !TargetASC || !SourceASC)
	{
		return;
	}

	FGameplayEffectSpecHandle BuffSpec = MakeOutgoingGameplayEffectSpec(BuffEffectClass);
	if (!BuffSpec.IsValid())
	{
		return;
	}

	// EffectContext에 Payload로 Niagara + Sound 설정
	if (BuffAuraEffectData)
	{
		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		ContextHandle.AddSourceObject(BuffAuraEffectData);
		BuffSpec.Data->SetContext(ContextHandle);
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(*BuffSpec.Data.Get());
}

void USFGA_Thrust_Salvation::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Thrust_Salvation::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	DisableCameraYawLimitsForActiveMode();
	ClearCameraMode();
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
