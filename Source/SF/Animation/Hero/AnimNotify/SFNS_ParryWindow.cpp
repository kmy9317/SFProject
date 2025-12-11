#include "SFNS_ParryWindow.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFNS_ParryWindow)

// ASC 찾는 헬퍼
static USFAbilitySystemComponent* FindSFASCFromActor(AActor* Owner)
{
	if (!Owner)
	{
		return nullptr;
	}

	// Lyra 스타일: Owner, Controller, PlayerState 등을 모두 뒤져서 ASC를 찾아줌
	if (UAbilitySystemComponent* ASCBase =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner))
	{
		return Cast<USFAbilitySystemComponent>(ASCBase);
	}

	return nullptr;
}

void USFNS_ParryWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ParryWindow] NotifyBegin Called : Owner=%s"),
		*Owner->GetName());

	if (!ParryWindowTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ParryWindow] ParryWindowTag is INVALID on %s"),
			*GetNameSafe(this));
		return;
	}

	USFAbilitySystemComponent* SFASC = FindSFASCFromActor(Owner);
	if (!SFASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ParryWindow] ASC NOT FOUND on BEGIN: %s"),
			*Owner->GetName());
		return;
	}

	SFASC->AddLooseGameplayTag(ParryWindowTag);

	UE_LOG(LogTemp, Warning,
		TEXT("[ParryWindow] BEGIN - Tag Added (%s) to %s"),
		*ParryWindowTag.ToString(),
		*Owner->GetName());
}

void USFNS_ParryWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ParryWindow] NotifyEnd Called : Owner=%s"),
		*Owner->GetName());

	if (!ParryWindowTag.IsValid())
	{
		return;
	}

	USFAbilitySystemComponent* SFASC = FindSFASCFromActor(Owner);
	if (!SFASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ParryWindow] ASC NOT FOUND on END: %s"),
			*Owner->GetName());
		return;
	}

	if (SFASC->HasMatchingGameplayTag(ParryWindowTag))
	{
		SFASC->RemoveLooseGameplayTag(ParryWindowTag);

		UE_LOG(LogTemp, Warning,
			TEXT("[ParryWindow] END - Tag Removed (%s) from %s"),
			*ParryWindowTag.ToString(),
			*Owner->GetName());
	}
}
