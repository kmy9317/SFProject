#include "SFGA_Hero_AreaHeal_C.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

#include "Character/SFCharacterBase.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/SkeletalMeshComponent.h"

USFGA_Hero_AreaHeal_C::USFGA_Hero_AreaHeal_C(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LightningEventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Skill.AreaHeal"));
}


//=====================무기 메쉬 찾기(사실 안쓸듯)==================
USkeletalMeshComponent* USFGA_Hero_AreaHeal_C::FindCurrentWeaponMesh(ASFCharacterBase* OwnerChar) const
{
	if (!OwnerChar) return nullptr;

	TArray<USceneComponent*> Children;
	OwnerChar->GetMesh()->GetChildrenComponents(true, Children);

	for (USceneComponent* Comp : Children)
	{
		if (USkeletalMeshComponent* Skel = Cast<USkeletalMeshComponent>(Comp))
		{
			//MainWeapon 태그가 붙은 스켈레탈을 무기로 인식
			if (Skel->ComponentTags.Contains("MainWeapon"))
				return Skel; 
		}
	}
	return nullptr;
}
//================================================================


//=====================스킬 실행=======================
void USFGA_Hero_AreaHeal_C::ActivateAbility(
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
	
	ASFCharacterBase* OwnerChar = GetSFCharacterFromActorInfo();
	if (!OwnerChar){ EndAbility(Handle,ActorInfo,ActivationInfo,true,true); return; }

	//=====================Trail 생성=====================
	if (SwordTrailFX)
	{
		if (USkeletalMeshComponent* Mesh = FindCurrentWeaponMesh(OwnerChar))
		{
			TrailComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
				SwordTrailFX, Mesh, NAME_None,
				FVector::ZeroVector,FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,true,true);

			if(TrailComp) TrailComp->SetVariableFloat(TEXT("User.Fade"),1.f);

			FTimerDelegate Update;
			TWeakObjectPtr<USkeletalMeshComponent> Weak=Mesh;

			Update.BindLambda([this,Weak]()
			{
				if(!TrailComp||!Weak.IsValid())return;

				auto* M=Weak.Get();
				TrailComp->SetVariableVec3(TEXT("User.TrailStart"),M->GetSocketLocation("Trail_Start"));
				TrailComp->SetVariableVec3(TEXT("User.TrailEnd"),M->GetSocketLocation("Trail_End"));
			});

			OwnerChar->GetWorldTimerManager().SetTimer(TrailUpdateHandle,Update,0.01f,true);
		}
	}
	//====================================================
	
	//=====================Montage실행====================
	if(LightningMontage)
	{
		auto* M=UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this,"LM",LightningMontage);
		M->OnCompleted.AddDynamic(this,&USFGA_Hero_AreaHeal_C::OnMontageEnded);
		M->OnInterrupted.AddDynamic(this,&USFGA_Hero_AreaHeal_C::OnMontageEnded);
		M->OnCancelled.AddDynamic(this,&USFGA_Hero_AreaHeal_C::OnMontageEnded);
		M->ReadyForActivation();
	}
	//====================================================
	
	//=====================Event대기======================
	if(LightningEventTag.IsValid())
	{
		auto* E=UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this,LightningEventTag);
		if(E){E->EventReceived.AddDynamic(this,&USFGA_Hero_AreaHeal_C::OnLightningImpact);E->ReadyForActivation();}
	}
	//====================================================
}
//====================================================


//=====================FX Que 호출, 데미지 처리=====================
void USFGA_Hero_AreaHeal_C::OnLightningImpact(FGameplayEventData Payload)
{
	ASFCharacterBase* OwnerChar=GetSFCharacterFromActorInfo();
	if(!OwnerChar)return;

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	
	//공격 위치 계산
	FVector StrikePos=
		OwnerChar->GetActorLocation()+
		OwnerChar->GetActorForwardVector()*StrikeDistance;

	//지면 보정
	{
		FHitResult H;
		if(GetWorld()->LineTraceSingleByChannel(H,StrikePos+FVector(0,0,200),StrikePos-FVector(0,0,400),ECC_Visibility))
			StrikePos=H.ImpactPoint+FVector(0,0,5);
	}
	
	//FX 출력(GC)
	if(LightningCueTag.IsValid())
		if(auto* ASC=UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerChar))
		{
			FGameplayCueParameters P;
			P.Location=StrikePos;
			P.Instigator=OwnerChar;
			ASC->ExecuteGameplayCue(LightningCueTag,P);
		}
	
	//타격 판정
	TArray<FHitResult> HitResults;
	UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(),StrikePos+FVector(0,0,50),StrikePos-FVector(0,0,50),
		StrikeRadius,UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,{OwnerChar},EDrawDebugTrace::None,HitResults,true);

	if(HitResults.Num()==0)return;
	
	//ActorTag기반 태그 캐싱
	bool OP=OwnerChar->Tags.Contains("Player");
	bool OE=OwnerChar->Tags.Contains("Enemy");

	//데미지, 디버프 처리
	for(const FHitResult& Hit : HitResults)
	{
		AActor* HitActor=Hit.GetActor();
		if(!HitActor||HitActor==OwnerChar)continue;

		auto* Target=Cast<ASFCharacterBase>(HitActor);
		if(!Target)continue;

		bool TP=Target->Tags.Contains("Player");
		bool TE=Target->Tags.Contains("Enemy");

		//아군 제외
		if((OP&&TP)||(OE&&TE))continue;

		//데미지 처리
		ProcessHitResult(Hit,GetScaledBaseDamage(),nullptr);

		//디버프 GE 적용
		if(DebuffGE)
		{
			if(auto* ASC=UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
			{
				auto S=MakeOutgoingGameplayEffectSpec(DebuffGE,1.f);
				if(S.IsValid())ASC->ApplyGameplayEffectSpecToSelf(*S.Data.Get());
			}
		}
	}
}
//==================================================================


//================Montage 종료시 이벤트=================
void USFGA_Hero_AreaHeal_C::OnMontageEnded()
{
	EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,false);
}
//====================================================


//=====================EndAbility=====================
void USFGA_Hero_AreaHeal_C::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,bool bWasCancelled)
{
	//Trail Fade Out 처리
	if(TrailComp)
	{
		TWeakObjectPtr<UNiagaraComponent> W=TrailComp;
		float F=1.f;

		GetWorld()->GetTimerManager().SetTimer(
			TrailFadeHandle,
			[this,W,F]()mutable
			{
				if(!W.IsValid())return;
				F-=0.04f;W->SetVariableFloat(TEXT("User.Fade"),F);
				if(F<=0.f){W->Deactivate();W->DestroyComponent();TrailComp=nullptr;}
			},
			0.03f,true);
	}

	Super::EndAbility(Handle,ActorInfo,ActivationInfo,bReplicateEndAbility,bWasCancelled);
}
//====================================================
