// Enemy Base Attack Ability (Refactored & Organized)

#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "AI/Controller/SFEnemyController.h"
#include "Character/SFCharacterBase.h"
#include "Character/SFCharacterGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFGA_Enemy_BaseAttack)

USFGA_Enemy_BaseAttack::USFGA_Enemy_BaseAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ReplicationPolicy  = EGameplayAbilityReplicationPolicy::ReplicateYes;


	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Attacking);
	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_UsingAbility);


	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Dead);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Stunned);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Hit); 
	ActivationBlockedTags.AddTag(SFGameplayTags::Ability_Cooldown_Enemy_Attack);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Blocking);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Parried);

	bIsCancelable = true;
}

bool USFGA_Enemy_BaseAttack::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
		return false;
	

	if (!ActorInfo->IsNetAuthority())
		return true;
	
	const FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
	AActor* Self = ActorInfo->AvatarActor.Get();
	AActor* Target = GetCurrentTarget();

	if (!Spec || !Self || !Target)
		return false;

	return CheckRangeAndAngle(Self, Target, Spec);
}

void USFGA_Enemy_BaseAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
}

void USFGA_Enemy_BaseAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{

	if (HasAuthority(&ActivationInfo) && !bWasCancelled)
	{
		ApplyCooldown(Handle, ActorInfo, ActivationInfo);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


// Range ,Angle Check
bool USFGA_Enemy_BaseAttack::CheckRangeAndAngle(
	const AActor* Self, 
	const AActor* Target, 
	const FGameplayAbilitySpec* Spec) const
{
	if (!Self || !Target || !Spec)
		return false;
	
	// SetByCaller 값 추출 헬퍼
	auto GetValue = [&](const FGameplayTag& Tag, float Default) -> float
	{
		const float* Ptr = Spec->SetByCallerTagMagnitudes.Find(Tag);
		return Ptr ? *Ptr : Default;
	};

	const float AttackRange = GetValue(SFGameplayTags::Data_EnemyAbility_AttackRange, 200.f);
	const float MinAttackRange = GetValue(SFGameplayTags::Data_EnemyAbility_MinAttackRange, 0.f);
	const float AttackAngle = GetValue(SFGameplayTags::Data_EnemyAbility_AttackAngle, 45.f);
	
	// 거리 체크
	const float Distance = FVector::Dist(Self->GetActorLocation(), Target->GetActorLocation());
	if (Distance > AttackRange || Distance <= MinAttackRange)
		return false;
	
	// 각도 체크
	const FVector ToTarget = (Target->GetActorLocation() - Self->GetActorLocation()).GetSafeNormal();
	const float Dot = FVector::DotProduct(Self->GetActorForwardVector(), ToTarget);
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
	
	return Angle <= (AttackAngle * 0.5f);
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
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

	return Angle <= (GetAttackAngle() * 0.5f);
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

// AI Score
float USFGA_Enemy_BaseAttack::CalcAIScore(const FEnemyAbilitySelectContext& Context) const
{
	// Validation
	if (!Context.Self || !Context.Target || !Context.AbilitySpec)
		return -FLT_MAX;

	
	auto GetValueFromSpec = [&](const FGameplayTag& Tag, float DefaultValue) -> float
	{
		const float* ValuePtr = Context.AbilitySpec->SetByCallerTagMagnitudes.Find(Tag);
		return ValuePtr ? *ValuePtr : DefaultValue;
	};

	
	const float BaseDamage = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_BaseDamage, 0.f);
	const float Cooldown = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_Cooldown, 1.f);
	
	const float Dist = Context.DistanceToTarget;

	// 1. Min Range Check 
	const float MinAttackRange = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_MinAttackRange, 0.f);
	if (Dist <= MinAttackRange)
		return 0.f;

	//Angle Check with Blind Spot Exception
	// 근거리(300 미만)면 뒤에서도 공격 허용 (AI가 빠르게 회전)
	// 원거리면 각도 엄격히 체크
	const float BlindCheckRange = 300.0f;
	if (Dist > BlindCheckRange)
	{
		if (!IsWithinAttackAngle(Context))
			return 0.f;
	}

	// 3. Calculate Base Score
	float Score = 0.f;

	const bool bIsInRange = IsWithinAttackRange(Context);

	if (bIsInRange)
	{
		// 사거리 내 높은 기본 점수
		Score = 1500.f;

		// 최적 거리 보너스 
		const float AttackRange = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_AttackRange, 0.f);
		const float OptimalDistance = (AttackRange + MinAttackRange) * 0.5f;
		const float DistFromOptimal = FMath::Abs(Dist - OptimalDistance);
		const float MaxDeviation = FMath::Max((AttackRange - MinAttackRange) * 0.5f, 1.f);

		// 최적 거리에 가까울수록 높은 점수 
		const float DistanceScore = FMath::Clamp(1.0f - (DistFromOptimal / MaxDeviation), 0.f, 1.f);
		Score += DistanceScore * 500.f;
	}
	else
	{
		// 사거리 밖: 낮은 점수 
		Score = 500.f;
	}

	// 4. DPS 가중치 
	const float CooldownSafe = FMath::Max(Cooldown, 0.1f);
	Score += (BaseDamage / CooldownSafe) * 10.f;

	// 5. Ability별 추가 보정치 
	const float Modifier = CalcScoreModifier(Context);
	Score += Modifier;

	// 6. 우선순위 플래그 
	if (Context.bMustFirst)
		Score += 100000.f;

	return Score;
}

float USFGA_Enemy_BaseAttack::CalcScoreModifier(const FEnemyAbilitySelectContext& Context) const
{
	return 0.f;
}


// Damage 
void USFGA_Enemy_BaseAttack::ApplyDamageToTarget(
	AActor* Target,
	const FGameplayEffectContextHandle& ContextHandle)
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
	const FGameplayEffectContextHandle& ContextHandle)
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

// Knockback & Launch

void USFGA_Enemy_BaseAttack::ApplyKnockBackToTarget(AActor* Target, const FVector& HitLocation) const
{
	if (!Target)
		return;

	ASFCharacterBase* Instigator = GetSFCharacterFromActorInfo();
	if (!Instigator)
		return;

	FVector KnockBackDirection = FVector::UpVector;

	// GameplayEvent 데이터 생성
	FGameplayEventData EventData;
	EventData.Instigator = Instigator;
	EventData.Target = Target;
	EventData.EventMagnitude = 1.0f; // 넉백 강도 배율
	
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
		return;

	ASFCharacterBase* Instigator = GetSFCharacterFromActorInfo();
	if (!Instigator)
		return;

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

//Utility

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

// SetByCaller
float USFGA_Enemy_BaseAttack::GetSetByCallerValue(const FGameplayTag& Tag, float DefaultValue) const
{
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();

	if (!Spec)
		return DefaultValue;

	const float* ValuePtr = Spec->SetByCallerTagMagnitudes.Find(Tag);

	if (ValuePtr)
		return *ValuePtr;

	return DefaultValue;
}

float USFGA_Enemy_BaseAttack::GetBaseDamage() const
{
	return GetSetByCallerValue(SFGameplayTags::Data_EnemyAbility_BaseDamage, 10.f);
}

float USFGA_Enemy_BaseAttack::GetAttackRange() const
{
	return GetSetByCallerValue(SFGameplayTags::Data_EnemyAbility_AttackRange, 200.f);
}

float USFGA_Enemy_BaseAttack::GetMinAttackRange() const
{
	return GetSetByCallerValue(SFGameplayTags::Data_EnemyAbility_MinAttackRange, 0.f);
}

float USFGA_Enemy_BaseAttack::GetCooldown() const
{
	return GetSetByCallerValue(SFGameplayTags::Data_EnemyAbility_Cooldown, 1.f);
}

float USFGA_Enemy_BaseAttack::GetAttackAngle() const
{
	return GetSetByCallerValue(SFGameplayTags::Data_EnemyAbility_AttackAngle, 45.f);
}