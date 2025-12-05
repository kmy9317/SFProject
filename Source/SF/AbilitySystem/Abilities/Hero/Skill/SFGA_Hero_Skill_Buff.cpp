#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Hero_Skill_Buff.h"

#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFGA_Hero_Skill_Buff)

USFGA_Hero_Skill_Buff::USFGA_Hero_Skill_Buff(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

//===============================Ability 실행=========================================
void USFGA_Hero_Skill_Buff::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	//코스트 & 쿨타임 체크
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	//애니메이션 재생
	if (BuffMontage)
	{
		if (UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr)
		{
			AnimInstance->Montage_Play(BuffMontage);
		}
	}

	//이펙트 + 사운드 실행
	SpawnBuffVisualsAndAudio(ActorInfo);

	//실제 버프 로직 수행
	ApplyBuffEffectToTargets(Handle, ActorInfo, ActivationInfo);
}
//===================================================================================

//===============================이펙트 & 사운드=======================================
void USFGA_Hero_Skill_Buff::SpawnBuffVisualsAndAudio(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
		return;

	AActor* Avatar = ActorInfo->AvatarActor.Get();
	UWorld* World = Avatar->GetWorld();
	if (!World)
		return;

	//시전자 발밑 위치 계산
	FVector GroundLocation = Avatar->GetActorLocation();

	if (ACharacter* Character = Cast<ACharacter>(Avatar))
	{
		GroundLocation.Z -= Character->GetSimpleCollisionHalfHeight();
	}

	//이펙트 생성
	if (BuffParticleSystem)
	{
		UParticleSystemComponent* FXComp =
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BuffParticleSystem,
				GroundLocation
			);

		if (FXComp)
		{
			ActiveFXComponents.Add(FXComp);
		}
	}

	//사운드 실행
	if (BuffSound)
	{
		UAudioComponent* AudioComp =
			UGameplayStatics::SpawnSoundAtLocation(
				World,
				BuffSound,
				Avatar->GetActorLocation()
			);

		if (AudioComp)
		{
			ActiveFXComponents.Add(AudioComp);
		}
	}
}
//===================================================================================

//===============================버프 효과 적용=========================================
void USFGA_Hero_Skill_Buff::ApplyBuffEffectToTargets(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	//자식에서 오버라이드해서 사용
}
//===================================================================================

//===============================Ability 종료=========================================
void USFGA_Hero_Skill_Buff::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	//현재 존재하는 이펙트 & 사운드 정리
	for (USceneComponent* Comp : ActiveFXComponents)
	{
		if (IsValid(Comp))
		{
			Comp->Deactivate();
			Comp->DestroyComponent();
		}
	}
	ActiveFXComponents.Empty();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
//===================================================================================