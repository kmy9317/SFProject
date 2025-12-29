#include "SFWorldInteractable.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "SFLogChannels.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "Character/SFCharacterBase.h"
#include "Net/UnrealNetwork.h"

ASFWorldInteractable::ASFWorldInteractable(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
}

void ASFWorldInteractable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bWasConsumed);
}

void ASFWorldInteractable::OnInteractActiveStarted(AActor* Interactor)
{
	if (!IsValid(Interactor))
	{
		return;
	}
	
	if (HasAuthority())
	{
		CachedInteractors.Add(Interactor);
	}

	K2_OnInteractActiveStarted(Interactor);
}

void ASFWorldInteractable::OnInteractActiveEnded(AActor* Interactor)
{
	if (!IsValid(Interactor))
	{
		return;
	}
	
	if (HasAuthority())
	{
		CachedInteractors.RemoveSingleSwap(Interactor);
	}

	K2_OnInteractActiveEnded(Interactor);
}

void ASFWorldInteractable::OnInteractionSuccess(AActor* Interactor)
{
	if (!IsValid(Interactor))
	{
		return;
	}
	
	if (HasAuthority())
	{
		// 일회성 상호작용 객체인 경우 소모 처리 및 다른 상호작용자들의 상호작용 취소
		if (bShouldConsume)
		{
			bWasConsumed = true;

			TArray<TWeakObjectPtr<AActor>> TargetInteractors = MoveTemp(CachedInteractors);

			// 현재 상호작용 중인 다른 플레이어들의 상호작용을 취소
			for (TWeakObjectPtr<AActor>& TargetInteractor : TargetInteractors)
			{
				if (ASFCharacterBase* TargetCharacter = Cast<ASFCharacterBase>(TargetInteractor.Get()))
				{
					// 성공한 상호작용자는 제외
					if (Interactor == TargetCharacter)
					{
						continue;
					}

					// 다른 플레이어의 상호작용 어빌리티 취소
					if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetCharacter))
					{
						FGameplayTagContainer CancelAbilitiesTag;
						CancelAbilitiesTag.AddTag(SFGameplayTags::Ability_Interact_Active);
						ASC->CancelAbilities(&CancelAbilitiesTag);
					}
				}
			}
		}
		else
		{
			// 반복 사용 가능한 객체의 경우 캐시만 정리
			CachedInteractors.Empty();
		}
	}
	
	K2_OnInteractionSuccess(Interactor);
}

bool ASFWorldInteractable::CanInteraction(const FSFInteractionQuery& InteractionQuery) const
{
	if (!ISFInteractable::CanInteraction(InteractionQuery))
	{
		return false;
	}
	
	return bShouldConsume ? (bWasConsumed == false) : true;
}

void ASFWorldInteractable::OnRep_WasConsumed()
{
	// 기본 구현 (필요 시)
	UE_LOG(LogSF, Warning, TEXT("bWasConsumed changed: %d"), bWasConsumed);
}

int32 ASFWorldInteractable::GetActiveInteractorCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& Interactor : CachedInteractors)
	{
		if (Interactor.IsValid())
		{
			++Count;
		}
	}
	return Count;
}



