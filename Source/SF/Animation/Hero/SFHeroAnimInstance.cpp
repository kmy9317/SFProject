// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Hero/SFHeroAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/SFCharacterBase.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"

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
		
		// 애니메이션 시스템 초기화 시점에 이미 장착된 장비가 있다면 레이어 링크 수행
		if (USkeletalMeshComponent* OwnerMesh = GetOwningComponent())
		{
			UAnimInstance* MainAnimInst = OwnerMesh->GetAnimInstance();
            
			// 조건 확인
			if (MainAnimInst == this)
			{
				if (USFEquipmentComponent* EquipComp = USFEquipmentComponent::FindEquipmentComponent(OwnerCharacter))
				{
					EquipComp->ReapplyItemAnimLayers();
				}
			}
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

	if (MovementComponent)
	{
		// 입력(가속도)이 있을 때만 True. (감속 중일 때는 False가 됨 -> Stop 모션 발동)
		bShouldMove = (MovementComponent->GetCurrentAcceleration().SizeSquared() > 0.0f);
	}
	else
	{
		bShouldMove = false;
	}

	if (MovementComponent)
	{
		bIsCrouching = MovementComponent->IsCrouching();
	}
	else
	{
		bIsCrouching = false;
	}
	
	if (bShouldMove)
	{
		LastMoveSpeed = GroundSpeed;
	}

	// GameplayTag 확인
	// OwnerCharacter가 ASFCharacterBase 타입이므로, 직접 GetSFAbilitySystemComponent() 호출 가능
	if (const USFAbilitySystemComponent* ASC = OwnerCharacter->GetSFAbilitySystemComponent())
	{
		// 전투 상태 태그 확인
		bIsAttacking = ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Attacking);
		bIsBlocking = ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Blocking);
		bIsStunned = ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Stunned);
		bIsSprinting = ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Sprint);
	}
}