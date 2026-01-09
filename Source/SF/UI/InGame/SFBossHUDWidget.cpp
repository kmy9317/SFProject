#include "UI/InGame/SFBossHUDWidget.h"

#include "AbilitySystem/Attributes/Enemy/SFPrimarySet_Enemy.h"
#include "Character/Enemy/SFEnemy.h"
#include "Team/SFTeamTypes.h"


void USFBossHUDWidget::InitializeBoss(ACharacter* BossActor)
{
	ASFEnemy* EnemyBoss = Cast<ASFEnemy>(BossActor);
	
	if (!BossActor && !EnemyBoss)
	{
		SetVisibility(ESlateVisibility::Hidden); return;
	}
	SetVisibility(ESlateVisibility::Visible);
	BP_OnNameChanged(EnemyBoss->GetName());

	UAbilitySystemComponent* ASC = EnemyBoss->GetAbilitySystemComponent();

	if (ASC)
	{
		BossASC = ASC;
		bool bFound = false;
		CurrentHealth = ASC->GetGameplayAttributeValue(USFPrimarySet_Enemy::GetHealthAttribute(), bFound);
		CurrentMaxHealth = ASC->GetGameplayAttributeValue(USFPrimarySet_Enemy::GetMaxHealthAttribute(), bFound);

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

void USFBossHUDWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	CurrentMaxHealth = Data.NewValue;
	float Percent = (CurrentMaxHealth > 0.0f) ? (CurrentHealth / CurrentMaxHealth) : 0.0f;
	BP_OnHealthChanged(CurrentHealth, CurrentMaxHealth, Percent);
}

void USFBossHUDWidget::OnDamageRecived(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue > 0.0f)
	{
		BP_OnDamageRecived(Data.NewValue);
	}
}
