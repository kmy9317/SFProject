#include "SFGA_Hero_Skill_Buff.h"

#include "Components/CapsuleComponent.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/SFCharacterBase.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/Hero/Skill/SkillActor/SFBuffArea.h"

USFGA_Hero_Skill_Buff::USFGA_Hero_Skill_Buff(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

//========================ActivateAbility========================
void USFGA_Hero_Skill_Buff::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitCheck(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	//몽타주 재생
	if (BuffMontage && ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this,
				NAME_None,
				BuffMontage
			);

		MontageTask->OnCompleted.AddDynamic(this, &USFGA_Hero_Skill_Buff::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &USFGA_Hero_Skill_Buff::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &USFGA_Hero_Skill_Buff::OnMontageInterrupted);
		MontageTask->OnBlendOut.AddDynamic(this, &USFGA_Hero_Skill_Buff::OnMontageBlendOut);

		MontageTask->ReadyForActivation();
	}

	//GameplayEvent 대기 (노티파이에서 StartEventTag 쏴주면 됨)
	if (StartEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* EventTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this,
				StartEventTag,
				NULL,
				true,
				true
			);

		EventTask->EventReceived.AddDynamic(this, &USFGA_Hero_Skill_Buff::OnReceivedSkillEvent);
		EventTask->ReadyForActivation();
	}
}
//===============================================================


//====================OnReceivedSkillEvent====================
void USFGA_Hero_Skill_Buff::OnReceivedSkillEvent(FGameplayEventData Payload)
{
	UAbilitySystemComponent* ASC =
		CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	
	if (!ASC)
		return;

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	
	//실제 스킬 로직 호출(BuffArea 장판 소환)
	OnSkillEventTriggered();
}
//===========================================================


//===========================BuffArea 액터 소환=============================
void USFGA_Hero_Skill_Buff::OnSkillEventTriggered_Implementation()
{
	if (!BuffAreaClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !CurrentActorInfo)
	{
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return;
	}
	
	//BuffArea 액터 소환 위치 지정
	FVector SpawnLoc = Avatar->GetActorLocation();

	float HalfHeight = 0.f;

	//Pawn인지 확인 후 Capsule 정보 가져오기
	if (ACharacter* Char = Cast<ACharacter>(Avatar))
	{
		if (auto* Capsule = Char->GetCapsuleComponent())
		{
			HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	//캐릭터 중심 → 발바닥 위치로 이동
	SpawnLoc.Z -= HalfHeight;

	//FX 추가 위치 보정
	SpawnLoc.Z += 3.f;
	
	//BuffArea 액터 소환
	FActorSpawnParameters Params;
	Params.Owner = Avatar;
	Params.Instigator = Cast<APawn>(Avatar);

	ASFBuffArea* AreaActor = World->SpawnActor<ASFBuffArea>(
		BuffAreaClass,
		SpawnLoc,
		FRotator::ZeroRotator,
		Params
	);

	if (AreaActor)
	{
		AreaActor->InitializeArea(ASC);
	}
}
//=====================================================================


//========================Montage Delegates========================
void USFGA_Hero_Skill_Buff::OnMontageInterrupted()
{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Hero_Skill_Buff::OnMontageBlendOut()
{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Hero_Skill_Buff::OnMontageCompleted()
{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
//=================================================================


//==========================EndAbility==========================
void USFGA_Hero_Skill_Buff::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
//==============================================================
