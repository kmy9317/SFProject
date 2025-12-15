#include "SFGA_Hero_Parrying_C.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"

void USFGA_Hero_Parrying_C::OnParryEventReceived(FGameplayEventData Payload)
{
	Super::OnParryEventReceived(Payload);

	//===============이유는 모르겠지만 한번 더 해야 제대로 판정됨==================
	AActor* OwnerActor = GetAvatarActorFromActorInfo();
	if (!OwnerActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] OwnerActor NULL"));
		return;
	}

	// 1) 공격자 찾기
	AActor* InstigatorActor = Payload.ContextHandle.GetOriginalInstigator();
	if (!InstigatorActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] Failed GetOriginalInstigator"));
		InstigatorActor = const_cast<AActor*>(Payload.Instigator.Get());
	}

	if (!InstigatorActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] Instigator NULL - Cannot calculate angle"));
		return;
	}

	//방향 계산
	FVector OwnerLocation = OwnerActor->GetActorLocation();
	FVector InstigatorLocation = InstigatorActor->GetActorLocation();
	FVector DirToInstigator = (InstigatorLocation - OwnerLocation).GetSafeNormal();

	FVector OwnerForward = OwnerActor->GetActorForwardVector().GetSafeNormal();

	//각도 계산
	float Dot = FVector::DotProduct(OwnerForward, DirToInstigator);
	float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));

	//전방 180° 에서 공격이 들어왔는지 판단
	bool bInFront = (AngleDeg <= 90.0f);

	UE_LOG(LogTemp, Warning, TEXT("[Parry] Angle=%.2f | Dot=%.2f | InFront(180deg)=%d"),
		AngleDeg, Dot, bInFront);

	// 4) 후방 공격 → 패링 실패
	if (!bInFront)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] FAILED - Attack from BEHIND"));
		return;
	}
	//========================================================================
	
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}
	
	//패링 성공 시 GA 교체 로직 수행
	ReplaceAbilityOnServer();
	
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
}

void USFGA_Hero_Parrying_C::ReplaceAbilityOnServer()
{
	if (!ReplacementAbilityClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Parry_C] ReplacementAbilityClass is NULL"));
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	FGameplayAbilitySpec* CurrentSpec = ASC->FindAbilitySpecFromHandle(CurrentSpecHandle);
	if (!CurrentSpec)
	{
		return;
	}

	const int32 AbilityLevel = CurrentSpec->Level;
	
	//InputTag에 해당하는 GA 제거
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(ReplacementInputTag))
		{
			ASC->ClearAbility(Spec.Handle);
			break;
		}
	}
	
	//지정한 강화 GA 부여
	USFGameplayAbility* AbilityCDO = ReplacementAbilityClass->GetDefaultObject<USFGameplayAbility>();
	FGameplayAbilitySpec NewSpec(AbilityCDO, AbilityLevel);
	NewSpec.GetDynamicSpecSourceTags().AddTag(ReplacementInputTag);
	
	ASC->GiveAbility(NewSpec);
	
	UE_LOG(LogTemp, Warning,
		TEXT("[Parry_C] Ability Replaced -> %s (Level %d, Slot %s)"),
		*GetNameSafe(ReplacementAbilityClass),
		AbilityLevel,
		*ReplacementInputTag.ToString()
	);
}
