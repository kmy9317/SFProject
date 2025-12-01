// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Hero/SFHeroAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/SFCharacterBase.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/SFCharacterGameplayTags.h"

USFHeroAnimInstance::USFHeroAnimInstance()
{
	GroundSpeed = 0.f;
	LocomotionDirection = 0.0f;
	bShouldMove = false;
	bIsFalling = false;
	bIsCrouching = false;
}

void USFHeroAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// 오너 캐릭터 캐싱 및 ASFCharacterBase로 캐스팅
	OwnerCharacter = Cast<ASFCharacterBase>(GetOwningActor());
	if (OwnerCharacter)
	{
		MovementComponent = OwnerCharacter->GetCharacterMovement(); // ASFCharacterBase도 UCharacter를 상속하므로 GetCharacterMovement 사용 가능

		// AbilitySystemComponent 찾아서 초기화
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerCharacter))
		{
			InitializeWithAbilitySystem(ASC);
		}
	}
}

void USFHeroAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);
	GameplayTagPropertyMap.Initialize(this, ASC);
}

void USFHeroAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	// OwnerCharacter가 유효한지 확인 (MovementComponent는 OwnerCharacter에서 가져오므로 OwnerCharacter만 확인)
	if (!OwnerCharacter) return;

	// ASFCharacterBase에서 계산된 값들을 직접 가져와 사용
	GroundSpeed = OwnerCharacter->GetGroundSpeed();
	LocomotionDirection = OwnerCharacter->GetDirection(); // ASFCharacterBase의 GetDirection() 사용
	bIsFalling = OwnerCharacter->IsFalling(); // ASFCharacterBase의 IsFalling() 사용

	// bShouldMove는 GroundSpeed로 대체 가능하지만, 기존 로직이 있다면 유지
	// 현재는 ASFCharacterBase에서 가속도 정보를 직접 노출하지 않으므로 기존 로직 유지
	// if (MovementComponent)
	// {
	// 	bShouldMove = (GroundSpeed > 3.0f) || (MovementComponent->GetCurrentAcceleration().SizeSquared() > 0.0f);
	// }
	// else
	// {
	// 	bShouldMove = (GroundSpeed > 3.0f);
	// }

	// 앉기 여부 (ASFCharacterBase에 IsCrouching이 있다면 사용, 없으면 MovementComponent 사용)
	// 현재 ASFCharacterBase에는 IsCrouching 게터가 없으므로 MovementComponent 사용
	if (MovementComponent)
	{
		bIsCrouching = MovementComponent->IsCrouching();
	}
	else
	{
		bIsCrouching = false;
	}


	// 5. GameplayTag 확인 (스레드 안전하게)
	// OwnerCharacter가 ASFCharacterBase 타입이므로, 직접 GetSFAbilitySystemComponent() 호출 가능
	if (const USFAbilitySystemComponent* ASC = OwnerCharacter->GetSFAbilitySystemComponent())
	{
		// 전투 상태 태그 확인
		bIsAttacking = ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Attacking);
		bIsBlocking = ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Blocking);
		bIsStunned = ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Stunned);
	}
}