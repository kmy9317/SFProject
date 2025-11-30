// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/Component/SFEnemyMovementComponent.h"

#include "SFStateReactionComponent.h"
#include "AI/SFAIGameplayTags.h"
#include "Character/SFCharacterGameplayTags.h"
#include "GameFramework/Character.h"

void USFEnemyMovementComponent::InitializeMovementComponent()
{
	MappingStateFunction();
	USFStateReactionComponent* StateReactionComponent = USFStateReactionComponent::FindStateReactionComponent(GetOwner());
	if (IsValid(StateReactionComponent))
	{
		StateReactionComponent->OnStateStart.AddDynamic(this, &ThisClass::OnStateStart);
		StateReactionComponent->OnStateEnd.AddDynamic(this, &ThisClass::OnStateEnd);
	}
	
}

void USFEnemyMovementComponent::InternalDisableMovement()
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return;

	UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement();
	if (!MoveComp) return;

	// 안전한 디세이블 방식
	PreviousMovementMode = MoveComp->MovementMode;

	MoveComp->StopMovementImmediately();

	MoveComp->SetMovementMode(MOVE_None);
}

void USFEnemyMovementComponent::InternalEnableMovement()
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return;

	UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement();
	if (!MoveComp) return;

	// 이전 모드가 정상이라면 복구
	if (PreviousMovementMode != MOVE_None)
	{
		MoveComp->SetMovementMode(PreviousMovementMode);
	}
	else
	{
		// 기본값 WALK
		MoveComp->SetMovementMode(MOVE_Walking);
	}
}

void USFEnemyMovementComponent::MappingStateFunction()
{
	StateStartMap.Add(SFGameplayTags::Character_State_Parried, [this]() { StartStateParried(); });
	StateStartMap.Add(SFGameplayTags::Character_State_Stunned, [this]() { StartStateStunned(); });
	StateStartMap.Add(SFGameplayTags::Character_State_Groggy, [this]() { StartStateGroggy(); });
	StateStartMap.Add(SFGameplayTags::Character_State_Hit, [this]() { StartStateHitReact(); });
	StateStartMap.Add(SFGameplayTags::Character_State_Knockback, [this]() { StartStateKnockback(); });
	StateStartMap.Add(SFGameplayTags::Character_State_Knockdown, [this]() { StartStateKnockdown(); });
	StateStartMap.Add(SFGameplayTags::Character_State_Dead, [this]() { StartStateDead(); });

	StateEndMap.Add(SFGameplayTags::Character_State_Parried, [this]() { EndStateParried(); });
	StateEndMap.Add(SFGameplayTags::Character_State_Stunned, [this]() { EndStateStunned(); });
	StateEndMap.Add(SFGameplayTags::Character_State_Groggy, [this]() { EndStateGroggy(); });
	StateEndMap.Add(SFGameplayTags::Character_State_Hit, [this]() { EndStateHitReact(); });
	StateEndMap.Add(SFGameplayTags::Character_State_Knockback, [this]() { EndStateKnockback(); });
	StateEndMap.Add(SFGameplayTags::Character_State_Knockdown, [this]() { EndStateKnockdown(); });
	StateEndMap.Add(SFGameplayTags::Character_State_Dead, [this]() { EndStateDead(); });

}


void USFEnemyMovementComponent::OnStateStart(FGameplayTag StateTag)
{
	if (StateStartMap.Contains(StateTag))
	{
		StateStartMap[StateTag]();
	}
}

void USFEnemyMovementComponent::OnStateEnd(FGameplayTag StateTag)
{
	if (StateEndMap.Contains(StateTag))
	{
		StateEndMap[StateTag]();
	}
}

void USFEnemyMovementComponent::StartStateParried()
{
	InternalDisableMovement();
}

void USFEnemyMovementComponent::EndStateParried()
{
	InternalEnableMovement();
}

void USFEnemyMovementComponent::StartStateStunned()
{
	InternalDisableMovement();
}

void USFEnemyMovementComponent::EndStateStunned()
{
	InternalEnableMovement();
}

void USFEnemyMovementComponent::StartStateGroggy()
{
	InternalDisableMovement();
}

void USFEnemyMovementComponent::EndStateGroggy()
{
	InternalEnableMovement();
}

void USFEnemyMovementComponent::StartStateHitReact()
{
	InternalDisableMovement();
}

void USFEnemyMovementComponent::EndStateHitReact()
{
	InternalEnableMovement();
}

void USFEnemyMovementComponent::StartStateKnockback()
{
	InternalDisableMovement();
}

void USFEnemyMovementComponent::EndStateKnockback()
{
	InternalEnableMovement();
}

void USFEnemyMovementComponent::StartStateKnockdown()
{
	InternalDisableMovement();
}

void USFEnemyMovementComponent::EndStateKnockdown()
{
	InternalEnableMovement();
}

void USFEnemyMovementComponent::StartStateDead()
{
	InternalDisableMovement();
}

void USFEnemyMovementComponent::EndStateDead()
{
	InternalEnableMovement();
}
