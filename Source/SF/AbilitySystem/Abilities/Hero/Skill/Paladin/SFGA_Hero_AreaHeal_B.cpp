#include "SFGA_Hero_AreaHeal_B.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/SFCharacterBase.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"

USFGA_Hero_AreaHeal_B::USFGA_Hero_AreaHeal_B(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

//====================== Ability 실행 ======================
void USFGA_Hero_AreaHeal_B::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid()) return;

	FixedOrigin = ActorInfo->AvatarActor->GetActorLocation();

	//GameplayEvent(Notify) 대기
	if (HealEventTag.IsValid())
	{
		if (auto* Task = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HealEventTag))
		{
			Task->EventReceived.AddDynamic(this, &USFGA_Hero_AreaHeal_B::OnHealEventReceived);
			Task->ReadyForActivation();
		}
	}
}
//=========================================================


//=================Notify GameplayEvent 수신=================
void USFGA_Hero_AreaHeal_B::OnHealEventReceived(FGameplayEventData Payload)
{
	const FGameplayAbilityActorInfo* ActorInfo = CurrentActorInfo;
	if (!ActorInfo) return;

	// FX/사운드 부모 함수 여기서 실행됨
	Super::SpawnBuffVisualsAndAudio(ActorInfo);

	StartAreaSkill();
}
//==========================================================


//=======================실제 스킬 시작=======================
void USFGA_Hero_AreaHeal_B::StartAreaSkill()
{
	if (!GetWorld()) return;

	//장판 FX 생성
	if (AreaNiagaraSystem)
	{
		AreaNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), AreaNiagaraSystem, FixedOrigin, FRotator::ZeroRotator
		);
	}

	//즉시 1회 Tick
	UpdateArea(CurrentActorInfo);

	//지속 Tick 시작
	GetWorld()->GetTimerManager().SetTimer(
		AoECheckTimer,
		[this]() { UpdateArea(CurrentActorInfo); },
		CheckInterval,
		true);

	//지속시간 후 종료
	GetWorld()->GetTimerManager().SetTimer(
		EndAreaTimerHandle,
		this,
		&USFGA_Hero_AreaHeal_B::EndAreaSkill,
		AreaDuration,
		false);
}
//==========================================================


//===================진입 체크 & 효과 적용====================
void USFGA_Hero_AreaHeal_B::UpdateArea(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!GetWorld() || !ActorInfo) return;

	TArray<AActor*> All;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASFCharacterBase::StaticClass(), All);

	const float R2 = AreaRadius * AreaRadius;
	TSet<ASFCharacterBase*> Inside;

	for (AActor* A : All)
	{
		ASFCharacterBase* C = Cast<ASFCharacterBase>(A);
		if (!C) continue;

		if (!bIncludeSelf && C == ActorInfo->AvatarActor.Get()) continue;

		if (FVector::DistSquared(FixedOrigin, C->GetActorLocation()) > R2) continue;

		Inside.Add(C);
	}

	//새로 진입한 대상 처리
	for (ASFCharacterBase* C : Inside)
	{
		if (ActiveAreaEffects.Contains(C)) continue;
		if (!C->ActorHasTag("Player")) continue;

		auto* ASC = Cast<USFAbilitySystemComponent>(C->GetAbilitySystemComponent());
		if (!ASC) continue;

		auto Spec = ASC->MakeOutgoingSpec(BuffEffectClass, BuffLevel, ASC->MakeEffectContext());
		if (!Spec.IsValid()) continue;

		auto Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		if (!Handle.IsValid()) continue;

		ActiveAreaEffects.Add(C, Handle);

		//무적 오라 FX 적용
		if (InvincibleAuraNiagara)
		{
			UNiagaraComponent* FX = UNiagaraFunctionLibrary::SpawnSystemAttached(
				InvincibleAuraNiagara, C->GetMesh(), NAME_None,
				FVector::ZeroVector, FRotator::ZeroRotator,
				EAttachLocation::SnapToTargetIncludingScale, false);

			if (FX) ActiveAreaAuras.Add(C, FX);
		}
	}

	//범위 벗어날 시 적용
	for (auto It = ActiveAreaEffects.CreateIterator(); It; ++It)
	{
		ASFCharacterBase* C = It.Key();
		if (Inside.Contains(C)) continue;

		if (auto ASC = Cast<USFAbilitySystemComponent>(C->GetAbilitySystemComponent()))
			ASC->RemoveActiveGameplayEffect(It.Value());

		if (auto AuraPtr = ActiveAreaAuras.Find(C))
		{
			if (auto FX = *AuraPtr)
			{
				FX->Deactivate();
				FX->DestroyComponent();
			}
			ActiveAreaAuras.Remove(C);
		}

		It.RemoveCurrent();
	}
}
//===============================================================


//=============================종료===============================
void USFGA_Hero_AreaHeal_B::EndAreaSkill()
{
	//타이머 정리
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AoECheckTimer);
		GetWorld()->GetTimerManager().ClearTimer(EndAreaTimerHandle);
	}
	
	//GE 제거
	for (auto& P : ActiveAreaEffects)
		if (auto ASC = Cast<USFAbilitySystemComponent>(P.Key->GetAbilitySystemComponent()))
			ASC->RemoveActiveGameplayEffect(P.Value);

	ActiveAreaEffects.Empty();

	//FX 제거
	for (auto& P : ActiveAreaAuras)
	{
		if (auto FX = P.Value)
		{
			FX->Deactivate();
			FX->DestroyComponent();
		}
	}
	ActiveAreaAuras.Empty();

	if (AreaNiagaraComponent)
	{
		AreaNiagaraComponent->Deactivate();
		AreaNiagaraComponent->DestroyComponent();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
//==================================================================