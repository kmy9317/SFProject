#include "SFGA_Hero_AreaHeal_A.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

#include "Character/SFCharacterBase.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

USFGA_Hero_AreaHeal_A::USFGA_Hero_AreaHeal_A(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

//============================Ability 실행================================
void USFGA_Hero_AreaHeal_A::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid()) return;
	FixedOrigin = ActorInfo->AvatarActor->GetActorLocation();

	//Anim 노티파이 GameplayEvent 대기
	if (HealEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* Task = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HealEventTag);
		if (Task)
		{
			Task->EventReceived.AddDynamic(this, &USFGA_Hero_AreaHeal_A::OnHealEventReceived);
			Task->ReadyForActivation();
		}
	}
}
//========================================================================

//========================GameplayEvent 수신 이벤트========================
void USFGA_Hero_AreaHeal_A::OnHealEventReceived(FGameplayEventData Payload)
{
	const FGameplayAbilityActorInfo* ActorInfo = CurrentActorInfo;
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid()) return;

	FixedOrigin = ActorInfo->AvatarActor->GetActorLocation();

	Super::SpawnBuffVisualsAndAudio(ActorInfo);

	StartArea();
}
//========================================================================

//=============================영역 스킬 시작===============================
void USFGA_Hero_AreaHeal_A::StartArea()
{
	const FGameplayAbilityActorInfo* ActorInfo = CurrentActorInfo;
	if (!GetWorld() || !ActorInfo) return;

	//즉시 1회 적용
	UpdateAreaEffect(ActorInfo);

	//Tick 실행
	GetWorld()->GetTimerManager().SetTimer(
		AoECheckTimer,
		[this, ActorInfo]() { UpdateAreaEffect(ActorInfo); },
		CheckInterval,
		true);

	//지속시간 후 종료
	GetWorld()->GetTimerManager().SetTimer(
		EndAreaTimerHandle,
		this,
		&USFGA_Hero_AreaHeal_A::EndArea,
		AreaDuration,
		false);
}
//=====================================================================

//======================영역 판정 + GE/FX 적용=======================
void USFGA_Hero_AreaHeal_A::UpdateAreaEffect(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!GetWorld() || !ActorInfo) return;

	TArray<AActor*> AllCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASFCharacterBase::StaticClass(), AllCharacters);

	const float RadiusSq = AreaRadius * AreaRadius;
	TSet<ASFCharacterBase*> InsideNow;

	for (AActor* A : AllCharacters)
	{
		ASFCharacterBase* Char = Cast<ASFCharacterBase>(A);
		if (!Char) continue;

		if (!bIncludeSelf && Char == ActorInfo->AvatarActor.Get()) continue;

		if (FVector::DistSquared(FixedOrigin, Char->GetActorLocation()) > RadiusSq)
			continue;

		InsideNow.Add(Char);
	}

	//새로 들어온 캐릭터 처리
	for (ASFCharacterBase* Char : InsideNow)
	{
		if (ActiveAreaEffects.Contains(Char)) continue;
		if (!Char->ActorHasTag("Player")) continue;

		auto* ASC = Cast<USFAbilitySystemComponent>(Char->GetAbilitySystemComponent());
		if (!ASC) continue;

		auto Ctx = ASC->MakeEffectContext();
		Ctx.AddSourceObject(this);

		auto Spec = ASC->MakeOutgoingSpec(BuffEffectClass, BuffLevel, Ctx);
		if (!Spec.IsValid()) continue;

		auto Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		if (!Handle.IsValid()) continue;

		ActiveAreaEffects.Add(Char, Handle);

		//FX
		if (AreaAuraNiagara)
		{
			UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAttached(
				AreaAuraNiagara, Char->GetMesh(), NAME_None,
				FVector::ZeroVector, FRotator::ZeroRotator,
				EAttachLocation::SnapToTargetIncludingScale, false);

			if (NC) ActiveAreaAuras.Add(Char, NC);
		}
	}

	//범위 벗어난 캐릭터 처리
	for (auto It = ActiveAreaEffects.CreateIterator(); It; ++It)
	{
		ASFCharacterBase* Char = It.Key();
		if (InsideNow.Contains(Char)) continue;

		if (auto* ASC = Cast<USFAbilitySystemComponent>(Char->GetAbilitySystemComponent()))
			ASC->RemoveActiveGameplayEffect(It.Value());

		if (UNiagaraComponent** FX = ActiveAreaAuras.Find(Char))
		{
			if (UNiagaraComponent* C = *FX)
			{
				C->Deactivate();
				C->DestroyComponent();
			}
			ActiveAreaAuras.Remove(Char);
		}
		It.RemoveCurrent();
	}
}
//====================================================================

//==============================종료=================================
void USFGA_Hero_AreaHeal_A::EndArea()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AoECheckTimer);
		GetWorld()->GetTimerManager().ClearTimer(EndAreaTimerHandle);
	}

	for (auto& P : ActiveAreaEffects)
		if (auto* ASC = Cast<USFAbilitySystemComponent>(P.Key->GetAbilitySystemComponent()))
			ASC->RemoveActiveGameplayEffect(P.Value);

	for (auto& P : ActiveAreaAuras)
	{
		if (auto* C = P.Value)
		{
			C->Deactivate();
			C->DestroyComponent();
		}
	}

	ActiveAreaEffects.Empty();
	ActiveAreaAuras.Empty();

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
//=====================================================================