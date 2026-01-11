#include "UI/InGame/SFBossHUDWidget.h"

#include "AbilitySystem/Attributes/Enemy/SFPrimarySet_Enemy.h"
#include "Character/Enemy/SFEnemy.h"
#include "Team/SFTeamTypes.h"


void USFBossHUDWidget::InitializeBoss(ACharacter* BossActor)
{
	ASFEnemy* EnemyBoss = Cast<ASFEnemy>(BossActor);
	
	if (!BossActor && !EnemyBoss)
	{
		SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	BP_OnNameChanged(EnemyBoss->GetName());

	UAbilitySystemComponent* ASC = EnemyBoss->GetAbilitySystemComponent();

	if (ASC)
	{
		BossASC = ASC;
		bool bFound = false;
		CurrentHealth = ASC->GetGameplayAttributeValue(USFPrimarySet_Enemy::GetHealthAttribute(), bFound);
		CurrentMaxHealth = ASC->GetGameplayAttributeValue(USFPrimarySet_Enemy::GetMaxHealthAttribute(), bFound);

		if (CurrentMaxHealth <= 0.0f)
		{
			SetVisibility(ESlateVisibility::Hidden); 
		}
		else
		{
			SetVisibility(ESlateVisibility::Visible);
		}
		ASC->GetGameplayAttributeValueChangeDelegate(USFPrimarySet_Enemy::GetHealthAttribute())
		.AddUObject(this, &ThisClass::OnHealthChanged);
		ASC->GetGameplayAttributeValueChangeDelegate(USFPrimarySet_Enemy::GetMaxHealthAttribute())
		.AddUObject(this, &ThisClass::OnMaxHealthChanged);

		float Percent = (CurrentMaxHealth > 0.0f) ? (CurrentHealth / CurrentMaxHealth) : 0.0f;
		BP_OnHealthChanged(CurrentHealth, CurrentMaxHealth, Percent);

		ASC->GetGameplayAttributeValueChangeDelegate(USFPrimarySet_Enemy::GetDamageAttribute())
		.AddUObject(this, &ThisClass::OnDamageRecived);
	}
}

void USFBossHUDWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	CurrentHealth = Data.NewValue;
	float Percent = (CurrentMaxHealth > 0.0f) ? (CurrentHealth / CurrentMaxHealth) : 0.0f;
	BP_OnHealthChanged(CurrentHealth, CurrentMaxHealth, Percent);
}

// SFBossHUDWidget.cpp 수정
void USFBossHUDWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	// 1. 값을 먼저 업데이트합니다.
	CurrentMaxHealth = Data.NewValue;

	// 2. 업데이트된 값이 유효한지 확인합니다.
	if (CurrentMaxHealth > 0.0f) 
	{
		if (GetVisibility() != ESlateVisibility::Visible)
		{
			SetVisibility(ESlateVisibility::Visible);
			// 여기서 BP_OnStartFillAnimation() 같은 이벤트를 호출하면 더 좋습니다.
		}
       
		float Percent = (CurrentHealth / CurrentMaxHealth);
		BP_OnHealthChanged(CurrentHealth, CurrentMaxHealth, Percent);
	}
}

void USFBossHUDWidget::OnDamageRecived(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue > 0.0f)
	{
		BP_OnDamageRecived(Data.NewValue);
	}
}
