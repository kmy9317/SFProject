#include "AbilitySystem/Abilities/Hero/Skill/Paladin/SFGA_Hero_AreaHeal.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/SFCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFHeroSkillTags.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFGA_Hero_AreaHeal)

USFGA_Hero_AreaHeal::USFGA_Hero_AreaHeal(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilityTags.AddTag(SFGameplayTags::Ability_Skill_Secondary_Hero);

	//노티파이에서 사용할 기본 태그 (프로젝트 GameplayTag에 등록되어 있어야 함)
	HealEventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Skill.AreaHeal"));
}

//===============================Ability 실행=========================================
void USFGA_Hero_AreaHeal::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// ⭐ 부모 버프 로직 실행:
	// - CommitAbility (코스트/쿨타임)
	// - BuffMontage 재생
	// - SpawnBuffVisualsAndAudio 호출 (하지만 여긴 오버라이드해서 아무 것도 안 함)
	// - ApplyBuffEffectToTargets 호출 (AreaHeal에선 빈 함수)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	//시전 시점 기준 좌표 (실제 장판 시작은 태그 이벤트 시점에서 다시 세팅해도 됨)
	FixedOrigin = ActorInfo->AvatarActor->GetActorLocation();

	// AnimNotify → GameplayEvent 태그를 쏘면 그때부터 장판 시작
	if (HealEventTag.IsValid())
	{
		if (UAbilityTask_WaitGameplayEvent* EventTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HealEventTag))
		{
			EventTask->EventReceived.AddDynamic(this, &USFGA_Hero_AreaHeal::OnHealEventReceived);
			EventTask->ReadyForActivation();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[USFGA_Hero_AreaHeal] HealEventTag is INVALID. Define 'Event.Skill.AreaHeal' and AnimNotify."));
	}
}
//===================================================================================

//======================초기 이펙트/사운드 자동 재생 막기=============================
void USFGA_Hero_AreaHeal::SpawnBuffVisualsAndAudio(const FGameplayAbilityActorInfo* ActorInfo)
{
	//부모는 여기서 BuffParticleSystem + BuffSound를 즉시 재생함.
	//AreaHeal은 "태그 이벤트 시점"에만 재생해야 하므로
	//여기서는 일부러 아무 것도 하지 않는다.
}
//===================================================================================

//=========================== GameplayEvent 수신 시점 ===============================
void USFGA_Hero_AreaHeal::OnHealEventReceived(FGameplayEventData Payload)
{
	const FGameplayAbilityActorInfo* ActorInfo = CurrentActorInfo;
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
		return;

	//노티파이 발생 시점 위치를 기준으로 장판 고정
	FixedOrigin = ActorInfo->AvatarActor->GetActorLocation();

	//장판 / 버프 로직 시작
	StartAreaHeal(ActorInfo);

	//이 시점에 부모의 이펙트 & 사운드 재생 로직을 호출
	Super::SpawnBuffVisualsAndAudio(ActorInfo);
}
//===================================================================================

//========================== 힐 장판 / 버프 로직 시작부 ==============================
void USFGA_Hero_AreaHeal::StartAreaHeal(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
		return;

	//영역 시작과 동시에 한 번 체크
	UpdateAreaHeal();

	//CheckInterval마다 범위 체크
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			AreaTickTimerHandle,
			this,
			&USFGA_Hero_AreaHeal::UpdateAreaHeal,
			CheckInterval,
			true
		);

		//스킬 지속시간 종료 시점
		World->GetTimerManager().SetTimer(
			AreaDurationTimerHandle,
			this,
			&USFGA_Hero_AreaHeal::OnAreaDurationFinished,
			AreaDuration,
			false
		);
	}
}
//===================================================================================

//=============================스킬 지속시간 종료======================================
void USFGA_Hero_AreaHeal::OnAreaDurationFinished()
{
	//스킬(장판) 지속시간이 끝나면 Ability 종료 → 힐/FX 모두 정리
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
//===================================================================================

//=============================영역 진입 여부 체크======================================
void USFGA_Hero_AreaHeal::UpdateAreaHeal()
{
	const FGameplayAbilityActorInfo* ActorInfo = CurrentActorInfo;
	if (!ActorInfo || !BuffEffectClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	//진입한 캐릭터 모두 체크
	TArray<AActor*> AllCharacters;
	UGameplayStatics::GetAllActorsOfClass(World, ASFCharacterBase::StaticClass(), AllCharacters);

	const float RadiusSq = HealRadius * HealRadius;

	TSet<ASFCharacterBase*> InsideNow;

	for (AActor* Actor : AllCharacters)
	{
		ASFCharacterBase* TargetCharacter = Cast<ASFCharacterBase>(Actor);
		if (!TargetCharacter)
			continue;

		if (!bIncludeSelf && TargetCharacter == ActorInfo->AvatarActor.Get())
			continue;

		const FVector TargetLocation = TargetCharacter->GetActorLocation();

		if (FVector::DistSquared(FixedOrigin, TargetLocation) > RadiusSq)
			continue;

		InsideNow.Add(TargetCharacter);
	}
	
	//범위 안으로 새로 들어온 캐릭터 처리
	for (ASFCharacterBase* TargetCharacter : InsideNow)
	{
		if (ActiveHealEffects.Contains(TargetCharacter))
			continue;
		
		//ActorTag 기반 처리
		if (!TargetCharacter->ActorHasTag("Player"))
			continue;
		
		USFAbilitySystemComponent* TargetASC =
			Cast<USFAbilitySystemComponent>(TargetCharacter->GetAbilitySystemComponent());

		if (!TargetASC)
			continue;

		// GE 생성
		FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
		Context.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle =
			TargetASC->MakeOutgoingSpec(BuffEffectClass, BuffLevel, Context);

		if (!SpecHandle.IsValid())
			continue;

		FActiveGameplayEffectHandle ActiveHandle =
			TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

		if (!ActiveHandle.IsValid())
			continue;

		ActiveHealEffects.Add(TargetCharacter, ActiveHandle);
		
		// 힐 오라 FX 생성
		if (TargetHealNiagaraSystem)
		{
			UNiagaraComponent* AuraComp = nullptr;

			if (USkeletalMeshComponent* MeshComp = TargetCharacter->GetMesh())
			{
				AuraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
					TargetHealNiagaraSystem,
					MeshComp,
					NAME_None,
					FVector::ZeroVector,
					FRotator::ZeroRotator,
					EAttachLocation::SnapToTargetIncludingScale,
					false
				);
			}
			else
			{
				const FVector TargetLocation = TargetCharacter->GetActorLocation();

				AuraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					World,
					TargetHealNiagaraSystem,
					TargetLocation,
					TargetCharacter->GetActorRotation()
				);
			}

			if (AuraComp)
				ActiveHealAuras.Add(TargetCharacter, AuraComp);
		}
	}
	
	//범위 밖으로 나간 캐릭터 처리
	for (auto It = ActiveHealEffects.CreateIterator(); It; ++It)
	{
		ASFCharacterBase* TargetCharacter = It.Key();
		FActiveGameplayEffectHandle Handle = It.Value();

		if (InsideNow.Contains(TargetCharacter))
			continue; //안 나갔을 경우

		//GE 제거
		if (USFAbilitySystemComponent* TargetASC =
			Cast<USFAbilitySystemComponent>(TargetCharacter->GetAbilitySystemComponent()))
		{
			TargetASC->RemoveActiveGameplayEffect(Handle);
		}

		//오라 FX 제거
		if (UNiagaraComponent** AuraPtr = ActiveHealAuras.Find(TargetCharacter))
		{
			if (UNiagaraComponent* AuraComp = *AuraPtr)
			{
				if (IsValid(AuraComp))
				{
					AuraComp->Deactivate();
					AuraComp->DestroyComponent();
				}
			}
			ActiveHealAuras.Remove(TargetCharacter);
		}

		It.RemoveCurrent();
	}
}
//===================================================================================

void USFGA_Hero_AreaHeal::ApplyBuffEffectToTargets(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	//AreaHeal은 Tick 기반 영역 판정(UpdateAreaHeal)을 사용하므로
	//여기서는 별도 버프 적용을 하지 않음.
}
//===================================================================================

//===============================Ability 종료=========================================
void USFGA_Hero_AreaHeal::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	//타이머 정리
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AreaTickTimerHandle);
		World->GetTimerManager().ClearTimer(AreaDurationTimerHandle);
	}

	//남아있는 모든 힐 GE 제거 + 나이아가라 제거
	for (auto& Pair : ActiveHealEffects)
	{
		ASFCharacterBase* TargetCharacter = Pair.Key;
		FActiveGameplayEffectHandle& Handle = Pair.Value;

		if (TargetCharacter)
		{
			if (USFAbilitySystemComponent* TargetASC =
				Cast<USFAbilitySystemComponent>(TargetCharacter->GetAbilitySystemComponent()))
			{
				if (Handle.IsValid())
				{
					TargetASC->RemoveActiveGameplayEffect(Handle);
				}
			}
		}
	}
	ActiveHealEffects.Empty();

	for (auto& AuraPair : ActiveHealAuras)
	{
		if (UNiagaraComponent* AuraComp = AuraPair.Value)
		{
			if (IsValid(AuraComp))
			{
				AuraComp->Deactivate();
				AuraComp->DestroyComponent();
			}
		}
	}
	ActiveHealAuras.Empty();

	//부모쪽 FX & 사운드 정리도 같이 수행
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
//===================================================================================
