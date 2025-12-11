// Enemy Base Attack Ability (Refactored & Organized)

#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/Controller/SFEnemyController.h"
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

	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Attacking);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Dead);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Stunned);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Hit);
	ActivationBlockedTags.AddTag(SFGameplayTags::Ability_Cooldown_Enemy_Attack);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Blocking);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Parried);
	
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

// [수정] AI 점수 계산 로직 전면 개편
// - 사거리 밖이면 0점 반환 (필터링)
// - 거리가 유효 사거리에 적합할수록 높은 점수
// - 데미지/쿨타임 비중 조정
float USFGA_Enemy_BaseAttack::CalcAIScore(const FEnemyAbilitySelectContext& Context) const
{
	if (!Context.Self || !Context.Target || !Context.AbilitySpec)
		return -FLT_MAX;

	auto GetValueFromSpec = [&](const FGameplayTag& Tag, float DefaultValue) -> float
	{
		const float* ValuePtr = Context.AbilitySpec->SetByCallerTagMagnitudes.Find(Tag);
		return ValuePtr ? *ValuePtr : DefaultValue;
	};

	const float BaseDamage = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_BaseDamage, 10.f);
	const float AttackRange = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_AttackRange, 200.f);
	const float MinAttackRange = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_MinAttackRange, 0.f);
	const float Cooldown = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_Cooldown, 1.f);
	const float AttackAngle = GetValueFromSpec(SFGameplayTags::Data_EnemyAbility_AttackAngle, 45.f);

	const float Dist = Context.Self->GetDistanceTo(Context.Target);

	// 1. [최소 거리 체크] 너무 가까우면 공격 불가 (0점)
	// 예: 창(Spear)이라서 딱 붙으면 못 때림 -> 뒤로 물러나는 로직이 없다면 0점 처리
	if (Dist <= MinAttackRange)
	{
		return 0.f;
	}

	// 2. [각도 체크] (0점)
	if (ASFCharacterBase* Owner = Cast<ASFCharacterBase>(Context.Self))
	{
		const FVector ToTarget = (Context.Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
		const float Dot = FVector::DotProduct(Owner->GetActorForwardVector(), ToTarget);
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

		if (Angle > (AttackAngle * 0.5f))
		{
			return 0.f;
		}
	}

	float Score = 0.f;
	bool bIsInRange = (Dist <= AttackRange);

	// ====================================================
	// [핵심 수정] 상태별 점수 부여 (Ready vs Chase)
	// ====================================================
	if (bIsInRange)
	{
		// 상황 A: [공격 가능] 사거리 안임 -> 매우 높은 점수 부여
		// 당장 때릴 수 있는 스킬을 우선적으로 선택하도록 함 (1500점 베이스)
		Score = 1500.f;

		// 거리 적합성 보너스 (최대 사거리 끝자락일수록 점수 높음 -> 카이팅 유도)
		const float Span = FMath::Max(AttackRange - MinAttackRange, 1.f);
		const float DistRatio = (Dist - MinAttackRange) / Span; 
		Score += DistRatio * 500.f;
	}
	else
	{
		// 상황 B: [추격 필요] 사거리 밖임 -> 낮은 점수 부여 (하지만 0점은 아님!)
		// 강공격 쿨타임일 때, 약공격(사거리 150)이 여기서 선택되어야 "접근"을 시도함
		Score = 500.f;

		// (옵션) 사거리가 긴 스킬일수록 조금 더 점수를 줘서, 조금만 가도 되는 걸 선호하게 함
		// Score += AttackRange * 1.0f; 
	}

	// 3. [공통] 데미지/쿨타임(DPS) 보정
	// 사거리 상태가 같다면, 쿨타임 짧고 센 스킬을 선호
	const float CooldownSafe = FMath::Max(Cooldown, 0.1f);
	Score += (BaseDamage / CooldownSafe) * 10.f;

	// 4. [우선권]
	if (Context.bMustFirst)
		Score += 100000.f;

	return Score;
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
	
	FGameplayEffectSpecHandle SpecHandle =
		MakeOutgoingGameplayEffectSpec(DamageGameplayEffectClass, GetAbilityLevel());

	if (!SpecHandle.IsValid())
		return;

	SpecHandle.Data->SetContext(ContextHandle);
	
	SpecHandle.Data->SetSetByCallerMagnitude(
		SFGameplayTags::Data_Damage_BaseDamage,
		GetBaseDamage()
	);
	
	SourceASC->ApplyGameplayEffectSpecToTarget( *SpecHandle.Data.Get(), TargetASC
	);
}

ASFCharacterBase* USFGA_Enemy_BaseAttack::GetCurrentTarget() const
{
	if (ASFEnemyController* Ctrl = Cast<ASFEnemyController>(GetControllerFromActorInfo()))
		return Cast<ASFCharacterBase>(Ctrl->TargetActor);

	return nullptr;
}

void USFGA_Enemy_BaseAttack::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
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

void USFGA_Enemy_BaseAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}