// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Hero/SFHero.h"

#include "AbilitySystemComponent.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Component/SFHeroMovementComponent.h"
#include "Component/SFHeroWidgetComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Physics/SFCollisionChannels.h"
#include "Player/SFPlayerController.h"
#include "Player/SFPlayerState.h"
#include "Team/SFTeamTypes.h"

ASFHero::ASFHero(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USFHeroMovementComponent>(CharacterMovementComponentName))
{
	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidgetComponent->SetupAttachment(GetRootComponent());
	OverheadWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	OverheadWidgetComponent->SetDrawAtDesiredSize(true);
	OverheadWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

	HeroWidgetComponent = CreateDefaultSubobject<USFHeroWidgetComponent>(TEXT("HeroWidgetComponent"));
}

ASFPlayerController* ASFHero::GetSFPlayerController() const
{
	return Cast<ASFPlayerController>(Controller);
}

FGenericTeamId ASFHero::GetGenericTeamId() const
{
	if (const ASFPlayerState* PS = GetPlayerState<ASFPlayerState>())
	{
		return PS->GetGenericTeamId();
	}
	return FGenericTeamId(SFTeamID::Player);
}

FSFInteractionInfo ASFHero::GetPreInteractionInfo(const FSFInteractionQuery& InteractionQuery) const
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Downed))
		{
			return ReviveInteractionInfo;
		}
	}
	
	return FSFInteractionInfo();
}

bool ASFHero::CanInteraction(const FSFInteractionQuery& InteractionQuery) const
{
	if (!ISFInteractable::CanInteraction(InteractionQuery))
	{
		return false;
	}
	
	if (InteractionQuery.RequestingAvatar.Get() == this)
	{
		return false;
	}
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Downed))
	{
		return false;
	}
	
	return true;
}

void ASFHero::OnInteractActiveStarted(AActor* Interactor)
{
	if (!HasAuthority() || !IsValid(Interactor))
	{
		return;
	}
	
	CachedRevivers.AddUnique(Interactor);
}

void ASFHero::OnInteractActiveEnded(AActor* Interactor)
{
	if (!HasAuthority() || !IsValid(Interactor))
	{
		return;
	}
	
	CachedRevivers.RemoveSingleSwap(Interactor);
}

void ASFHero::OnInteractionSuccess(AActor* Interactor)
{
	// 필요 시 추가 처리
}

int32 ASFHero::GetActiveInteractorCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& Reviver : CachedRevivers)
	{
		if (Reviver.IsValid())
		{
			++Count;
		}
	}
	return Count;
}

void ASFHero::OnAbilitySystemInitialized()
{
	Super::OnAbilitySystemInitialized();

	if (HeroWidgetComponent)
	{
		HeroWidgetComponent->SetOverheadWidgetComponent(OverheadWidgetComponent);
		if (ASFPlayerState* PS = GetPlayerState<ASFPlayerState>())
		{
			HeroWidgetComponent->InitializeHeroStatus(GetAbilitySystemComponent(), PS);
		}
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		// 태그 이벤트 등록
		DownedTagDelegateHandle = ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Downed, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnDownedTagChanged);

		// 초기 상태 체크
		if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Downed))
		{
			OnDownedTagChanged(SFGameplayTags::Character_State_Downed, 1);
		}
	}
}

void ASFHero::OnAbilitySystemUninitialized()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (DownedTagDelegateHandle.IsValid())
		{
			ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Downed, EGameplayTagEventType::NewOrRemoved).Remove(DownedTagDelegateHandle);
			DownedTagDelegateHandle.Reset(); 
		}
	}
	
	Super::OnAbilitySystemUninitialized();
}

void ASFHero::OnDownedTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (InteractionBox)
	{

		ECollisionResponse NewResponse = (NewCount > 0) ? ECR_Block : ECR_Ignore;
		InteractionBox->SetCollisionResponseToChannel(SF_TraceChannel_Interaction, NewResponse);
	}
}
