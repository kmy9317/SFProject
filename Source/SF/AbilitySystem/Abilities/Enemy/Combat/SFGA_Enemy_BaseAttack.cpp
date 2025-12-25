// Enemy Base Attack Ability (Refactored & Organized)

#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "AI/Controller/SFEnemyController.h"
#include "Animation/Enemy/SFEnemyAnimInstance.h"
#include "Character/SFCharacterBase.h"
#include "Character/SFCharacterGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFGA_Enemy_BaseAttack)

USFGA_Enemy_BaseAttack::USFGA_Enemy_BaseAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AI Ability는 서버에서만
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ReplicationPolicy  = EGameplayAbilityReplicationPolicy::ReplicateYes;

	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Attacking);
	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_UsingAbility);

	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Attacking);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Dead);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Stunned);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Hit);
	ActivationBlockedTags.AddTag(SFGameplayTags::Ability_Cooldown_Enemy_Attack);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Blocking);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Parried);

	bIsCancelable = true;
}

void USFGA_Enemy_BaseAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (HasAuthority(&ActivationInfo))
	{
		SaveRotationSettings();

		if (ASFBaseAIController* AIC =
			Cast<ASFBaseAIController>(GetControllerFromActorInfo()))
		{
			AIC->SetRotationMode(EAIRotationMode::None);
		}
	}

	
	
}
void USFGA_Enemy_BaseAttack::EndAbility( const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!bWasCancelled)
	{
		ApplyCooldown(Handle, ActorInfo, ActivationInfo);
	}
	RestoreRotationSettings();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

float USFGA_Enemy_BaseAttack::GetSetByCallerValue(const FGameplayTag& Tag, float DefaultValue) const
{
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();

	if (!Spec)
	{
		return DefaultValue;
	}

	const float* ValuePtr = Spec->SetByCallerTagMagnitudes.Find(Tag);

	if (ValuePtr)
	{
		return *ValuePtr;
	}

	return DefaultValue;
}
float USFGA_Enemy_BaseAttack::GetBaseDamage() const
{
	return GetSetByCallerValue(SFGameplayTags::Data_EnemyAbility_BaseDamage,     10.f);
}
float USFGA_Enemy_BaseAttack::GetAttackRange() const
{
	return GetSetByCallerValue(SFGameplayTags::Data_EnemyAbility_AttackRange,    200.f);
}
float USFGA_Enemy_BaseAttack::GetMinAttackRange() const
{
	return GetSetByCallerValue(SFGameplayTags::Data_EnemyAbility_MinAttackRange,  0.f);
}
float USFGA_Enemy_BaseAttack::GetCooldown() const
{
	return GetSetByCallerValue(SFGameplayTags::Data_EnemyAbility_Cooldown,        1.f);
}
float USFGA_Enemy_BaseAttack::GetAttackAngle() const
{
	return GetSetByCallerValue(SFGameplayTags::Data_EnemyAbility_AttackAngle,     45.f);
}

bool USFGA_Enemy_BaseAttack::IsWithinAttackRange(const AActor* Target) const
{
	const ASFCharacterBase* Owner = GetSFCharacterFromActorInfo();
	if (!Owner || !Target) return false;

	const float Dist = Owner->GetDistanceTo(Target);
	return (Dist <= GetAttackRange() && Dist > GetMinAttackRange());
}

bool USFGA_Enemy_BaseAttack::IsWithinAttackAngle(const AActor* Target) const
{
	const ASFCharacterBase* Owner = GetSFCharacterFromActorInfo();
	if (!Owner || !Target) return false;

	const FVector ToTarget = (Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
	const float Dot = FVector::DotProduct(Owner->GetActorForwardVector(), ToTarget);
	const float Angle  = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

	return Angle <= (GetAttackAngle() * 0.5f);
}

bool USFGA_Enemy_BaseAttack::CanAttackTarget(const AActor* Target) const
{
	return IsWithinAttackRange(Target) && IsWithinAttackAngle(Target);
}


bool USFGA_Enemy_BaseAttack::IsWithinAttackRange(const FEnemyAbilitySelectContext& Context) const
{
	if (!Context.AbilitySpec) return false;

	
	auto GetValueFromSpec = [&](const FGameplayTag& Tag, float DefaultValue) -> float
	{
		const float* ValuePtr = Context.AbilitySpec->SetByCallerTagMagnitudes.Find(Tag);
		return ValuePtr ? *ValuePtr : DefaultValue;
	};

	const float AttackRange = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_AttackRange, 200.f);
	const float MinAttackRange = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_MinAttackRange, 0.f);


	const float Dist = Context.DistanceToTarget;
	return (Dist <= AttackRange && Dist > MinAttackRange);
}

bool USFGA_Enemy_BaseAttack::IsWithinAttackAngle(const FEnemyAbilitySelectContext& Context) const
{
	if (!Context.AbilitySpec) return false;

	
	auto GetValueFromSpec = [&](const FGameplayTag& Tag, float DefaultValue) -> float
	{
		const float* ValuePtr = Context.AbilitySpec->SetByCallerTagMagnitudes.Find(Tag);
		return ValuePtr ? *ValuePtr : DefaultValue;
	};

	const float AttackAngle = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_AttackAngle, 45.f);
	
	const float Angle = Context.AngleToTarget;
	return Angle <= (AttackAngle * 0.5f);
}

bool USFGA_Enemy_BaseAttack::CanAttackTarget(const FEnemyAbilitySelectContext& Context) const
{
	return IsWithinAttackRange(Context) && IsWithinAttackAngle(Context);
}

// [수정] AI 점수 계산 로직 전면 개편
// - 사거리 밖이면 0점 반환 (필터링)
// - 거리가 유효 사거리에 적합할수록 높은 점수
// - 데미지/쿨타임 비중 조정
float USFGA_Enemy_BaseAttack::CalcAIScore(const FEnemyAbilitySelectContext& Context) const
{
	// Validation
	if (!Context.Self || !Context.Target || !Context.AbilitySpec)
		return -FLT_MAX;

	// Helper lambda to get SetByCaller values
	auto GetValueFromSpec = [&](const FGameplayTag& Tag, float DefaultValue) -> float
	{
		const float* ValuePtr = Context.AbilitySpec->SetByCallerTagMagnitudes.Find(Tag);
		return ValuePtr ? *ValuePtr : DefaultValue;
	};

	// Get ability data from Spec
	const float BaseDamage = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_BaseDamage, 0.f);
	const float AttackRange = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_AttackRange, 0.f);
	const float MinAttackRange = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_MinAttackRange, 0.f);
	const float Cooldown = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_Cooldown, 1.f);
	const float AttackAngle = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_AttackAngle, 0.f);

	
	const float Dist = Context.DistanceToTarget;
	const float Angle = Context.AngleToTarget;

	
	if (Dist <= MinAttackRange)
	{
		return 0.f;
	}

	// 2. Angle Check with Blind Spot Exception
	// If target is very close (< 300), allow attack even from behind (AI will rotate)
	// If target is far, strictly check angle
	const float BlindCheckRange = 300.0f;

	if (Dist > BlindCheckRange)
	{
		// ✅ Use helper function with Context
		if (!IsWithinAttackAngle(Context))
		{
			return 0.f;
		}
	}
	// Close targets: skip angle check (AI can rotate quickly)

	// 3. Calculate Base Score
	float Score = 0.f;

	// ✅ Use helper function with Context
	bool bIsInRange = IsWithinAttackRange(Context);

	if (bIsInRange)
	{
		// In Range: High base score
		Score = 1500.f;

		// Optimal Distance Bonus (prefer middle of range)
		const float OptimalDistance = (AttackRange + MinAttackRange) * 0.5f;
		const float DistFromOptimal = FMath::Abs(Dist - OptimalDistance);
		const float MaxDeviation = FMath::Max((AttackRange - MinAttackRange) * 0.5f, 1.f);

		// Closer to optimal = higher score (0.0 ~ 1.0)
		const float DistanceScore = FMath::Clamp(1.0f - (DistFromOptimal / MaxDeviation), 0.f, 1.f);
		Score += DistanceScore * 500.f;
	}
	else
	{
		// Out of Range: Low score (but not 0, to encourage approach)
		Score = 500.f;
	}

	// 4. DPS Weighting (Damage / Cooldown)
	const float CooldownSafe = FMath::Max(Cooldown, 0.1f);
	Score += (BaseDamage / CooldownSafe) * 10.f;

	// 5. Apply ability-specific modifier (for Boss abilities)
	float Modifier = CalcScoreModifier(Context);



	Score += Modifier;  // 양수 Modifier만 더함

	// 6. Priority Flag (must execute first)
	if (Context.bMustFirst)
		Score += 100000.f;

	return Score;
}

float USFGA_Enemy_BaseAttack::CalcScoreModifier(const FEnemyAbilitySelectContext& Context) const
{
	return 0.f;
}

void USFGA_Enemy_BaseAttack::ApplyKnockBackToTarget(AActor* Target, const FVector& HitLocation) const
{
	if (!Target)
	{
		return;
	}

	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (!Dragon)
	{
		return;
	}

	FVector KnockBackDirection = FVector::UpVector;

	// GameplayEvent 데이터 생성
	FGameplayEventData EventData;
	EventData.Instigator = Dragon;
	EventData.Target = Target;
	EventData.EventMagnitude = 1.0f; // 넉백 강도 배율

	// KnockBack 방향을 ContextHandle에 저장
	FHitResult HitResult;
	HitResult.ImpactPoint = Target->GetActorLocation();
	HitResult.ImpactNormal = KnockBackDirection;
	EventData.ContextHandle.AddHitResult(HitResult, true);

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

	if (TargetASC)
	{
		TargetASC->HandleGameplayEvent(
			SFGameplayTags::GameplayEvent_Knockback,
			&EventData
		);
	}
}

void USFGA_Enemy_BaseAttack::ApplyLaunchToTarget(AActor* Target, const FVector& LaunchDirection, float Magnitude) const
{
	if (!Target)
	{
		return;
	}
	ASFCharacterBase* Instigator = GetSFCharacterFromActorInfo();
	if (!Instigator)
	{
		return;
	}
	FGameplayEventData EventData;
	EventData.Instigator = Instigator;
	EventData.Target = Target;
	EventData.EventMagnitude = Magnitude;

	FHitResult HitResult;
	HitResult.ImpactPoint = Target->GetActorLocation();
	HitResult.ImpactNormal = LaunchDirection;
	EventData.ContextHandle.AddHitResult(HitResult, true);
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (TargetASC)
	{
		TargetASC->HandleGameplayEvent(
			SFGameplayTags::GameplayEvent_Launched,
			&EventData
		);
	}
}

void USFGA_Enemy_BaseAttack::ApplyDamageToTarget(
	AActor* Target,
	const FGameplayEffectContextHandle& ContextHandle
)
{
	if (!Target || !DamageGameplayEffectClass)
		return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
		return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
		return;

	// DamageGameplayEffectClass 사용 (SFDamageEffectExecCalculation 실행)
	FGameplayEffectSpecHandle SpecHandle =
		MakeOutgoingGameplayEffectSpec(DamageGameplayEffectClass, GetAbilityLevel());

	if (!SpecHandle.IsValid())
		return;

	SpecHandle.Data->SetContext(ContextHandle);

	// SetByCaller로 BaseDamage 전달 → Calculation에서 공격력/방어력/크리티컬 계산
	SpecHandle.Data->SetSetByCallerMagnitude(
		SFGameplayTags::Data_Damage_BaseDamage,
		GetBaseDamage()
	);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

void USFGA_Enemy_BaseAttack::ApplyRawDamageToTarget(
	AActor* Target,
	float RawDamage,
	const FGameplayEffectContextHandle& ContextHandle
)
{
	if (!Target || !RawDamageGameplayEffectClass)
		return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
		return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
		return;
	
	FGameplayEffectSpecHandle SpecHandle =
		MakeOutgoingGameplayEffectSpec(RawDamageGameplayEffectClass, GetAbilityLevel());

	if (!SpecHandle.IsValid())
		return;

	SpecHandle.Data->SetContext(ContextHandle);
	
	SpecHandle.Data->SetSetByCallerMagnitude(
		SFGameplayTags::Data_Damage_BaseDamage,
		RawDamage
	);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

AActor* USFGA_Enemy_BaseAttack::GetCurrentTarget() const
{
	if (ISFAIControllerInterface* Interface = Cast<ISFAIControllerInterface>(GetControllerFromActorInfo()))
	{
		if (USFEnemyCombatComponent* CombatComp = Interface->GetCombatComponent())
		{
			return CombatComp->GetCurrentTarget();
		}
	}
	return nullptr;
}

void USFGA_Enemy_BaseAttack::CommitExecute( const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	ApplyCost(Handle, ActorInfo, ActivationInfo);
}

void USFGA_Enemy_BaseAttack::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!bHasCooldown)
		return;
	
	if (!CooldownGameplayEffectClass)
		return;

	FGameplayEffectSpecHandle SpecHandle =
		MakeOutgoingGameplayEffectSpec(CooldownGameplayEffectClass, GetAbilityLevel());

	if (!SpecHandle.IsValid())
		return;

	SpecHandle.Data->SetDuration(GetCooldown(), true);
	SpecHandle.Data->DynamicGrantedTags.AddTag(CoolDownTag);

	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
}

void USFGA_Enemy_BaseAttack::SaveRotationSettings()
{
	if (ASFCharacterBase* Character = GetSFCharacterFromActorInfo())
	{
		// MovementMode 저장
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			SavedMovementMode = MoveComp->MovementMode;
		}

		// RotationMode 저장
		if (ASFBaseAIController* AIC = Cast<ASFBaseAIController>(Character->GetController()))
		{
			SavedAiRotationMode = AIC->GetCurrentRotationMode();
		}

		// ⭐ AnimInstance는 Controller의 SetRotationMode(None)에서 자동 처리됨
		// 여기서 직접 호출하지 않음
	}
}

void USFGA_Enemy_BaseAttack::RestoreRotationSettings()
{
	if (ASFCharacterBase* Character = GetSFCharacterFromActorInfo())
	{
		// MovementMode 복원
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			if (MoveComp->MovementMode != SavedMovementMode)
			{
				MoveComp->SetMovementMode(SavedMovementMode);
			}
		}

		// RotationMode 복원 (이 때 Controller의 SetRotationMode가 AnimInstance 처리)
		if (ASFBaseAIController* AIC = Cast<ASFBaseAIController>(Character->GetController()))
		{
			if (AIC->GetCurrentRotationMode() != SavedAiRotationMode)
			{
				AIC->SetRotationMode(SavedAiRotationMode);
			}
		}

		// ⭐ AnimInstance는 Controller의 SetRotationMode에서 자동 처리됨
		// 여기서 직접 호출하지 않음
	}
}

