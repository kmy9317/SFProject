#include "UI/Common/SkillSlotBase.h"

#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Interface/SFChainedSkill.h"
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
	OverlayController->OnChainStateChanged.AddDynamic(this, &ThisClass::OnChainStateChanged);

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
		// 연계 스킬 체크 (CDO에서 정보만 가져옴)
		if (const ISFChainedSkill* ChainedSkill = Cast<ISFChainedSkill>(Spec->Ability))
		{
			// CDO에서 ComboStateEffectClass 가져오기
			TSubclassOf<UGameplayEffect> ComboStateClass = ChainedSkill->GetComboStateEffectClass();
            
			// UI가 가진 ASC로 직접 카운트 조회
			int32 CurrentChain = 0;
			if (ComboStateClass)
			{
				CurrentChain = ASC->GetGameplayEffectCount(ComboStateClass, nullptr);
			}

			CachedChainIndex = CurrentChain;
            
			if (UTexture2D* ChainIcon = ChainedSkill->GetChainIcon(CurrentChain))
			{
				if (Img_SkillIcon)
				{
					Img_SkillIcon->SetBrushFromTexture(ChainIcon);
				}
				return;
			}
		}
        
		// 일반 스킬
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
		float TotalDuration = GetActiveCooldownDuration(ASC, Spec->Ability);
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

float USkillSlotBase::GetActiveCooldownDuration(UAbilitySystemComponent* ASC, UGameplayAbility* Ability)
{
	const FGameplayTagContainer* CooldownTags = Ability->GetCooldownTags();
	if (!CooldownTags || CooldownTags->Num() == 0)
	{
		return 0.f;
	}

	FGameplayEffectQuery Query;
	Query.MakeQuery_MatchAnyOwningTags(*CooldownTags);
    
	TArray<FActiveGameplayEffectHandle> ActiveHandles = ASC->GetActiveEffects(Query);
	if (ActiveHandles.Num() > 0)
	{
		if (const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(ActiveHandles[0]))
		{
			if (const UGameplayEffect* GEDef = ActiveGE->Spec.Def)
			{
				float Duration = 0.f;
				GEDef->DurationMagnitude.GetStaticMagnitudeIfPossible(ActiveGE->Spec.GetLevel(), Duration);
				return Duration;
			}
		}
	}
	return 0.f;
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
		CachedChainIndex = INDEX_NONE; 
		InitializeSlot();
	}
	else if (!bGiven && AbilitySpecHandle == CachedAbilitySpecHandle)
	{
		CachedAbilitySpecHandle = FGameplayAbilitySpecHandle();
		CachedChainIndex = INDEX_NONE;
	}
}

void USkillSlotBase::OnChainStateChanged(FGameplayAbilitySpecHandle AbilitySpecHandle, int32 ChainIndex)
{
	// 이 슬롯의 어빌리티인지 확인
	if (AbilitySpecHandle != CachedAbilitySpecHandle)
	{
		return;
	}

	// 동일한 ChainIndex면 무시 (중복 방지)
	if (CachedChainIndex == ChainIndex)
	{
		return;
	}

	CachedChainIndex = ChainIndex;
	UpdateChainIcon(ChainIndex);
}

void USkillSlotBase::UpdateChainIcon(int32 ChainIndex)
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

	ISFChainedSkill* ChainedSkill = Cast<ISFChainedSkill>(Spec->Ability);
	if (!ChainedSkill)
	{
		return;
	}

	if (UTexture2D* ChainIcon = ChainedSkill->GetChainIcon(ChainIndex))
	{
		if (Img_SkillIcon)
		{
			Img_SkillIcon->SetBrushFromTexture(ChainIcon);
		}
	}
}


