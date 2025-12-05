// Enemy Base Attack Ability (Refactored & Organized)

#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/GameplayEffect/Enemy/EffectContext/FSFHitEffectContext.h"
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
}

float USFGA_Enemy_BaseAttack::GetSetByCallerValue(const FGameplayTag& Tag, float DefaultValue) const
{
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	const float* ValuePtr = Spec ? Spec->SetByCallerTagMagnitudes.Find(Tag) : nullptr;

	return ValuePtr ? *ValuePtr : DefaultValue;
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
	const float Dot        = FVector::DotProduct(Owner->GetActorForwardVector(), ToTarget);
	const float Angle      = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

	return Angle <= (GetAttackAngle() * 0.5f);
}

bool USFGA_Enemy_BaseAttack::CanAttackTarget(const AActor* Target) const
{
	return IsWithinAttackRange(Target) && IsWithinAttackAngle(Target);
}

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

	const float CooldownSafe = FMath::Max(Cooldown, 0.1f);

	float Score = 0.f;

	// Damage/Cooldown 비중
	Score += (BaseDamage / CooldownSafe) * 15.f;

	// 거리 가중치
	const float Dist = Context.Self->GetDistanceTo(Context.Target);
	if (Dist <= AttackRange && Dist > MinAttackRange)
	{
		const float Span = FMath::Max(AttackRange - MinAttackRange, 1.f);
		const float DistNorm = FMath::Clamp((Dist - MinAttackRange) / Span, 0.f, 1.f);
		Score += (1.f - DistNorm) * 10.f;
	}

	// 각도 가중치 
	if (ASFCharacterBase* Owner = Cast<ASFCharacterBase>(Context.Self))
	{
		const FVector ToTarget = (Context.Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
		const float Dot = FVector::DotProduct(Owner->GetActorForwardVector(), ToTarget);
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

		const float AngleNorm = FMath::Clamp(Angle / (AttackAngle * 0.5f), 0.f, 1.f);
		Score += (1.f - AngleNorm) * 5.f;
	}

	if (Context.bMustFirst)
		Score += 100000.f;

	return Score;
}

void USFGA_Enemy_BaseAttack::ApplyDamageToTarget(
	AActor* Target,
	const FVector& AttackDirection,
	const FVector& AttackLocation
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
	
	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	// 커스텀 Context로 캐스트
	if (FSFHitEffectContext* SFContext =
		static_cast<FSFHitEffectContext*>(ContextHandle.Get()))
	{
		SFContext->SetAttackDirection(AttackDirection);
		SFContext->SetAttackLocation(AttackLocation);
	}

	// Spec 생성
	FGameplayEffectSpecHandle SpecHandle =
		MakeOutgoingGameplayEffectSpec(DamageGameplayEffectClass, GetAbilityLevel());

	if (!SpecHandle.IsValid())
		return;

	// context 넣기
	SpecHandle.Data->SetContext(ContextHandle);

	// SetByCaller로 데미지 넣기
	SpecHandle.Data->SetSetByCallerMagnitude(
		SFGameplayTags::Data_Damage_BaseDamage,
		GetBaseDamage()
	);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
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
