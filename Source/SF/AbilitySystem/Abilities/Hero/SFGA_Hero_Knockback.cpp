// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGA_Hero_Knockback.h"

#include "Character/SFCharacterBase.h"
#include "Player/SFPlayerController.h"

void USFGA_Hero_Knockback::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!IsActive())
	{
		return;
	}
	
	if (ASFPlayerController* SFPlayerController = GetSFPlayerControllerFromActorInfo())
	{
		// 이동 입력 무시 (WASD 등의 이동 불가)
		SFPlayerController->SetIgnoreMoveInput(true);

		if (ASFCharacterBase* SFCharacter = GetSFCharacterFromActorInfo())
		{
			// 컨트롤러 Yaw 회전 비활성화 (마우스로 캐릭터 회전 불가)
			// 넉백 중에는 캐릭터가 밀리는 방향을 유지해야 함
			SFCharacter->bUseControllerRotationYaw = false;
		}
	}
}

void USFGA_Hero_Knockback::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ASFPlayerController* SFPC = GetSFPlayerControllerFromActorInfo())
	{
		// 이동 입력 다시 허용
		SFPC->SetIgnoreMoveInput(false);

		if (ASFCharacterBase* SFCharacter = GetSFCharacterFromActorInfo())
		{
			// TODO : 타게팅 모드였다면 컨트롤러 Yaw 회전 다시 활성화
			//SFCharacter->bUseControllerRotationYaw = true;
		}
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
