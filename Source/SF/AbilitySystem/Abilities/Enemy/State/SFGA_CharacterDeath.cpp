// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGA_CharacterDeath.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/Attributes/Hero/SFCombatSet_Hero.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Character/Enemy/SFEnemy.h"
#include "Character/Enemy/SFEnemyData.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/SFEnemyManagerComponent.h"
#include "GameModes/SFGameState.h"
#include "Item/SFDropFunctionLibrary.h"

USFGA_CharacterDeath::USFGA_CharacterDeath()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bServerRespectsRemoteAbilityCancellation = true;

	 
	CancelAbilitiesWithTag.AddTag(SFGameplayTags::Character_State_UsingAbility);
	CancelAbilitiesWithTag.AddTag(SFGameplayTags::Character_State_Attacking);
	
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = SFGameplayTags::GameplayEvent_Death;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Dead);
}

void USFGA_CharacterDeath::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* Avatar = GetAvatarActorFromActorInfo();
	
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (Avatar->HasAuthority())
	{

		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->CancelAllAbilities(this);
		}
		
		if (ACharacter* Character = Cast<ACharacter>(Avatar))
		{
			Character->StopAnimMontage();
		}
		
		FTimerDelegate TimerDel;
		TimerDel.BindUObject(this, &ThisClass::DeathEventAfterDelay);
		Avatar->GetWorldTimerManager().SetTimer(EventTimerHandle, TimerDel, EventTime, false);

		// TODO : 추후 적 전용 Death 어빌리티에서 구현할 필요 있어보임
		if (ASFEnemy* Enemy = Cast<ASFEnemy>(Avatar))
		{
			// 드롭 실행
			ExecuteDrop(Enemy);

			Enemy->CheckBossDeath();

			// EnemyManager 등록 해제
			if (ASFGameState* SFGameState = GetWorld()->GetGameState<ASFGameState>())
			{
				if (USFEnemyManagerComponent* EnemyManager = SFGameState->GetEnemyManager())
				{
					EnemyManager->UnregisterEnemy(Enemy);
				}
			}
		}
		
		// FTimerDelegate TimerDel;
		TimerDel.BindUObject(this, &ThisClass::DeathEventAfterDelay);
		Avatar->GetWorldTimerManager().SetTimer(EventTimerHandle, TimerDel, EventTime, false);
	}
}

void USFGA_CharacterDeath::DeathEventAfterDelay()
{
	DeathTimerEvent();
}

void USFGA_CharacterDeath::DeathTimerEvent()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (Avatar && Avatar->HasAuthority())
	{
		Avatar->Destroy();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_CharacterDeath::ExecuteDrop(ASFEnemy* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	USFPawnExtensionComponent* PawnExtComp = Enemy->FindComponentByClass<USFPawnExtensionComponent>();
	if (!PawnExtComp)
	{
		return;
	}

	const USFEnemyData* EnemyData = PawnExtComp->GetPawnData<USFEnemyData>();
	if (!EnemyData)
	{
		return;
	}

	float LuckValue = 0.f;
	FVector DropLocation = Enemy->GetActorLocation();

	// 기본 드롭 테이블
	if (EnemyData->DropTable)
	{
		USFDropFunctionLibrary::ExecuteAndSpawnDrops(GetWorld(), EnemyData->DropTable, LuckValue, DropLocation);
	}

	// 추가 드롭 테이블들
	for (const USFDropTable* AdditionalTable : EnemyData->AdditionalDropTables)
	{
		if (AdditionalTable)
		{
			USFDropFunctionLibrary::ExecuteAndSpawnDrops(GetWorld(), AdditionalTable, LuckValue, DropLocation);
		}
	}
}

float USFGA_CharacterDeath::GetKillerLuckValue(AActor* Killer) const
{
	if (!Killer)
	{
		return 0.f;
	}

	if (APawn* KillerPawn = Cast<APawn>(Killer))
	{
		if (APlayerState* PS = KillerPawn->GetPlayerState())
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS))
			{
				bool bFound = false;
				float Luck = ASC->GetGameplayAttributeValue(USFCombatSet_Hero::GetLuckAttribute(), bFound);
				if (bFound)
				{
					return Luck;
				}
			}
		}
	}

	return 0.f;
}




