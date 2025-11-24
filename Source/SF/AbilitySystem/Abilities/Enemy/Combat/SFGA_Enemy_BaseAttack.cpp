// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AI/Controller/SFEnemyController.h"
#include "Character/SFCharacterBase.h"
#include "Character/SFCharacterGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFGA_Enemy_BaseAttack)

USFGA_Enemy_BaseAttack::USFGA_Enemy_BaseAttack(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	// AI 어빌리티는 서버에서만 실행
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// 복제 활성화 (다른 클라이언트가 볼 수 있도록)
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;

	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Attacking);

	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Attacking);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Dead);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Stunned);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Hit);
	ActivationBlockedTags.AddTag(SFGameplayTags::Ability_Cooldown_Enemy_Attack);  // ✅ 글로벌 쿨타임
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Blocking);
}

bool USFGA_Enemy_BaseAttack::IsWithinAttackRange(const AActor* Target) const
{
	if (IsValid(GetSFCharacterFromActorInfo()))
	{
		return (GetSFCharacterFromActorInfo()->GetDistanceTo(Target) <= AttackRange && GetSFCharacterFromActorInfo()->GetDistanceTo(Target) >MinAttackRange);
	}
	return false; 
}

bool USFGA_Enemy_BaseAttack::IsWithinAttackAngle(const AActor* Target) const
{
	if (ASFCharacterBase* OwnerChar = GetSFCharacterFromActorInfo())
	{
		//방향 벡터 
		FVector ToTarget = (Target->GetActorLocation() - OwnerChar->GetActorLocation()).GetSafeNormal();
		FVector Forward = OwnerChar->GetActorForwardVector();

		// 두 벡터 사이 각도 계산
		float Dot = FVector::DotProduct(Forward, ToTarget);
		float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));

		return AngleDegrees <= AttackAngle*0.5f;
	}
	return false;
}

bool USFGA_Enemy_BaseAttack::CanAttackTarget(const AActor* Target) const
{
	return (IsWithinAttackRange(Target)&&IsWithinAttackAngle(Target));
}

void USFGA_Enemy_BaseAttack::ApplyDamageToTarget(AActor* Target, float DamageAmount)
{
	if (!IsValid(Target))
	{
		return;
	}

	
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
	{
		return;
	}
	
	if (!DamageGameplayEffectClass)
	{
		return;
	}

	// 실제 데미지 값 결정 (파라미터로 받은 값이 있으면 사용, 없으면 BaseDamage 사용)
	float FinalDamage = (DamageAmount > 0.0f) ? DamageAmount : BaseDamage;

	// GameplayEffect Spec 생성
	FGameplayEffectContextHandle EffectContext = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	EffectContext.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageGameplayEffectClass, GetAbilityLevel());

	if (SpecHandle.IsValid())
	{
		// SetByCaller 방식으로 데미지 값 설정 (GameplayEffect에서 SetByCaller로 데미지를 받도록 설정해야 함)
		// 또는 직접 Magnitude를 수정할 수도 있음
		SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), FinalDamage);

		// 타겟에게 GameplayEffect 적용
		GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

ASFCharacterBase* USFGA_Enemy_BaseAttack::GetCurrentTarget() const
{
	if (AController* Controller = GetControllerFromActorInfo())
	{
		ASFEnemyController* EnemyController = Cast<ASFEnemyController>(Controller);
		if (EnemyController && EnemyController->TargetActor)
		{
			return Cast<ASFCharacterBase>(EnemyController->TargetActor);
		}
	}
	return nullptr;
}

void USFGA_Enemy_BaseAttack::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle, 
	const FGameplayAbilityActorInfo* ActorInfo, 
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (CooldownGE)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
			CooldownGameplayEffectClass, 
			GetAbilityLevel()
		);
        
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetDuration(Cooldown, true);

			if (CoolDownTag.IsValid())
			{
				SpecHandle.Data->DynamicGrantedTags.AddTag(CoolDownTag);
			}
		
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}
	
}

void USFGA_Enemy_BaseAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
