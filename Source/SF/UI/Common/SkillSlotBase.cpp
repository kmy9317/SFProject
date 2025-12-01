#include "UI/Common/SkillSlotBase.h"

#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "UI/Controller/SFOverlayWidgetController.h"

void USkillSlotBase::NativeOnWidgetControllerSet()
{
	if (PB_Cooldown)
	{
		PB_Cooldown->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_CooldownCount)
	{
		Text_CooldownCount->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_KeyPrompt && !KeyPromptText.IsEmpty())
	{
		Text_KeyPrompt->SetText(KeyPromptText);
	}

	USFOverlayWidgetController* OverlayController = GetWidgetControllerTyped<USFOverlayWidgetController>();
	if (!OverlayController)
	{
		return;
	}

	// 어빌리티 변경 델리게이트 바인딩
	OverlayController->OnAbilityChanged.AddDynamic(this, &ThisClass::OnAbilityChanged);

	// 현재 보유한 어빌리티에서 매칭되는 것 찾기
	UAbilitySystemComponent* ASC = OverlayController->GetWidgetControllerParams().AbilitySystemComponent;
	if (ASC)
	{
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.GetDynamicSpecSourceTags().HasTagExact(SlotInputTag))
			{
				CachedAbilitySpecHandle = Spec.Handle;
				InitializeSlot();
				break;
			}
		}
	}
}

void USkillSlotBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (CachedAbilitySpecHandle.IsValid())
	{
		RefreshCooldown();
	}
}

void USkillSlotBase::InitializeSlot()
{
	USFOverlayWidgetController* OverlayController = GetWidgetControllerTyped<USFOverlayWidgetController>();
	if (!OverlayController)
	{
		return;
	}

	UAbilitySystemComponent* ASC = OverlayController->GetWidgetControllerParams().AbilitySystemComponent;
	FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(CachedAbilitySpecHandle) : nullptr;
	
	if (!Spec)
	{
		return;
	}

	if (const USFGameplayAbility* SFAbility = Cast<USFGameplayAbility>(Spec->Ability))
	{
		if (Img_SkillIcon && SFAbility->Icon)
		{
			Img_SkillIcon->SetBrushFromTexture(SFAbility->Icon);
		}
	}
}

void USkillSlotBase::RefreshCooldown()
{
	USFOverlayWidgetController* OverlayController = GetWidgetControllerTyped<USFOverlayWidgetController>();
	if (!OverlayController)
	{
		return;
	}

	UAbilitySystemComponent* ASC = OverlayController->GetWidgetControllerParams().AbilitySystemComponent;
	FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(CachedAbilitySpecHandle) : nullptr;
	
	if (!Spec || !Spec->Ability)
	{
		return;
	}

	float CooldownRemaining = Spec->Ability->GetCooldownTimeRemaining(ASC->AbilityActorInfo.Get());
	
	if (CooldownRemaining > 0.f)
	{
		float TotalDuration = 0.f;
		if (const UGameplayEffect* CooldownGE = Spec->Ability->GetCooldownGameplayEffect())
		{
			CooldownGE->DurationMagnitude.GetStaticMagnitudeIfPossible(Spec->Level, TotalDuration);
		}
		if (Text_CooldownCount)
		{
			Text_CooldownCount->SetText(FText::AsNumber(FMath::CeilToInt(CooldownRemaining)));
			Text_CooldownCount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}

		if (PB_Cooldown && TotalDuration > 0.f)
		{
			PB_Cooldown->SetPercent(CooldownRemaining / TotalDuration);
			PB_Cooldown->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
	else
	{
		if (Text_CooldownCount)
		{
			Text_CooldownCount->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (PB_Cooldown)
		{
			PB_Cooldown->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void USkillSlotBase::OnAbilityChanged(FGameplayAbilitySpecHandle AbilitySpecHandle, bool bGiven)
{
	USFOverlayWidgetController* OverlayController = GetWidgetControllerTyped<USFOverlayWidgetController>();
	if (!OverlayController)
	{
		return;
	}

	UAbilitySystemComponent* ASC = OverlayController->GetWidgetControllerParams().AbilitySystemComponent;
	if (!ASC)
	{
		return;
	}

	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(AbilitySpecHandle);
	
	if (bGiven && Spec && Spec->GetDynamicSpecSourceTags().HasTagExact(SlotInputTag))
	{
		CachedAbilitySpecHandle = AbilitySpecHandle;
		InitializeSlot();
	}
	else if (!bGiven && AbilitySpecHandle == CachedAbilitySpecHandle)
	{
		CachedAbilitySpecHandle = FGameplayAbilitySpecHandle();
	}
}

// void USkillSlotBase::SetSlotVisuals(UTexture2D* InIcon, const FText& InKeyText)
// {
// 	if (Img_SkillIcon) Img_SkillIcon->SetBrushFromTexture(InIcon);
// 	if (Text_KeyPrompt) Text_KeyPrompt->SetText(InKeyText);
//
// 	if (PB_Cooldown) PB_Cooldown->SetVisibility(ESlateVisibility::Hidden);
// 	if (Text_CooldownCount) Text_CooldownCount->SetVisibility(ESlateVisibility::Hidden);
//
// 	bIsOnCooldown = false;
// }
//
// void USkillSlotBase::StartCooldown(float InDuration)
// {
// 	// 이미 쿨타임 중이라면 즉시 종료
// 	if (bIsOnCooldown)
// 	{
// 		
// 		return;
// 	}
// 	
// 	if (InDuration <= 0.0f) return;
//
// 	CooldownTotalDuration = InDuration;
// 	CooldownEndTime = GetWorld()->GetTimeSeconds() + InDuration; // 목표 시간 설정 -> 쿨타임
// 	bIsOnCooldown = true;
//
// 	if (PB_Cooldown) PB_Cooldown->SetVisibility(ESlateVisibility::Visible);
// 	if (Text_CooldownCount) Text_CooldownCount->SetVisibility(ESlateVisibility::Visible);
// }


