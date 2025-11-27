// Fill out your copyright notice in the Description page of Project Settings.


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
	ActivationBlockedTags.AddTag(SFGameplayTags::Ability_Cooldown_Enemy_Attack); 
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

float USFGA_Enemy_BaseAttack::CalcAIScore(const FEnemyAbilitySelectContext& Context) const
{
	
	if (!Context.Self || !Context.Target)
	{
		return -FLT_MAX; 
	}
	float Score = 0.f;

	const float SafeCooldown = FMath::Max(Cooldown, 0.1f); // 0 나누기 방지
	const float DpsValue    = BaseDamage / SafeCooldown;   // "공격 효율"
	Score += DpsValue * 15.f; 

	// 거리 가중치: 사거리 안에서 가까울수록 점수
	if (IsWithinAttackRange(Context.Target))
	{
		const float RangeSpan = FMath::Max(AttackRange - MinAttackRange, 1.f); 
		float DistanceToTarget = Context.Self->GetDistanceTo(Context.Target);
		float DistNorm = (DistanceToTarget - MinAttackRange) / RangeSpan; // 0~1
		DistNorm = FMath::Clamp(DistNorm, 0.f, 1.f);  // 0~1 사이로 정규화 
		const float DistanceWeight = 1.f - DistNorm;// 거리 가중치를 만들어 버림 
		Score += DistanceWeight * 10.f; // 거리 영향도
	}

	// 이건 각도 가중치  -> 정면에 있을 수록 점수가 높아진다  
	if (ASFCharacterBase* OwnerChar = GetSFCharacterFromActorInfo())
	{
		const FVector ToTarget = (Context.Target->GetActorLocation() - OwnerChar->GetActorLocation()).GetSafeNormal();
		const float   Dot = FVector::DotProduct(OwnerChar->GetActorForwardVector(), ToTarget); // 내적으로 현재 각을 구함 
		const float   AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
		float AngleNorm        = FMath::Clamp(AngleDeg / (AttackAngle * 0.5f), 0.f, 1.f); // 0~1 -> 일단 반각으로 해서 0~1로 
		const float AngleWeight = 1.f - AngleNorm; 
		Score += AngleWeight * 5.f;	//데미지 가중치 -> 데미지 / 쿨타임 해서 

	}
	
	// 5) bMustFirst 상황이면 무조건 먼저 하도록 
	if (Context.bMustFirst)
	{
		Score += 100000.f; 
	}

	return Score;
}

void USFGA_Enemy_BaseAttack::ApplyDamageToTarget(AActor* Target)
{
	if (!IsValid(Target)) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC) return;
    
	if (!DamageGameplayEffectClass) return;

	FGameplayEffectContextHandle EffectContext = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	EffectContext.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
		DamageGameplayEffectClass, 
		GetAbilityLevel()
	);

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(
		  SFGameplayTags::Data_Damage_BaseDamage, // 주소
		  BaseDamage
	  );
		
		GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(
			*SpecHandle.Data.Get(), 
			TargetASC
		);

		UE_LOG(LogTemp, Warning, TEXT("Damage Applied! Spec: %s"), *SpecHandle.Data->ToSimpleString());

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

void USFGA_Enemy_BaseAttack::ApplyCooldown( const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	
	if (CooldownGameplayEffectClass)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
			CooldownGameplayEffectClass, 
			GetAbilityLevel()
		);
        
		if (SpecHandle.IsValid())
		{
		
			SpecHandle.Data->SetDuration(Cooldown, true);
			
			SpecHandle.Data->DynamicGrantedTags.AddTag(CoolDownTag);
            
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
