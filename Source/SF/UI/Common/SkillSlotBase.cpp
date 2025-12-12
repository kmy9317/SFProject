#include "UI/Common/SkillSlotBase.h"

#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
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
	if (Img_SkillBorder)
	{
		Img_SkillBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		if (UMaterialInstanceDynamic* DMI = Img_SkillBorder->GetDynamicMaterial())
		{
			DMI->SetScalarParameterValue(FName("Percent"), 1.0f);
		}
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
		RefreshCooldown();			//쿨타임 갱신
		RefreshActiveDuration();    // 지속시간 갱신
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

void USkillSlotBase::RefreshActiveDuration()
{
	// 1. 컨트롤러 및 ASC 유효성 검사
	USFOverlayWidgetController* OverlayController = GetWidgetControllerTyped<USFOverlayWidgetController>();
	if (!OverlayController) return;
	
	UAbilitySystemComponent* ASC = OverlayController->GetWidgetControllerParams().AbilitySystemComponent;
	if (!ASC) return;

	// 2. 현재 지속시간(Remaining)과 전체시간(Total)을 받아올 변수
	float RemainingTime = 0.f;
	float TotalDuration = 0.f;

	// 3. 현재 스킬이 '활성화(Active)' 상태인지 체크
	bool bIsActive = GetCurrentSkillActiveDuration(ASC, RemainingTime, TotalDuration);

	if (Img_SkillBorder)
	{
		// 머티리얼 인스턴스 가져오기
		UMaterialInstanceDynamic* DMI = Img_SkillBorder->GetDynamicMaterial();
		if (!DMI) return;
		
		if (bIsActive && TotalDuration > 0.f)
		{
			// [상태 1] 스킬 발동 중: 시간에 따라 줄어듦
			float PercentValue = FMath::Clamp(RemainingTime / TotalDuration, 0.f, 1.f);
			DMI->SetScalarParameterValue(FName("Percent"), PercentValue);
		}
		else
		{
			// [상태 2] 평상시: 숨기는 게 아니라(Collapsed X), 꽉 채워서 테두리 유지
			DMI->SetScalarParameterValue(FName("Percent"), 1.0f);

			// 혹시라도 숨겨져 있었다면 다시 보이게 (방어 코드)
			if (Img_SkillBorder->GetVisibility() == ESlateVisibility::Collapsed)
			{
				Img_SkillBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
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

	FGameplayEffectQuery const Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(*CooldownTags);
	TArray<FActiveGameplayEffectHandle> ActiveHandles = ASC->GetActiveEffects(Query);
	
	if (ActiveHandles.Num() > 0)
	{
		if (const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(ActiveHandles[0]))
		{
			float Duration = 0.f;
			ActiveGE->Spec.Def->DurationMagnitude.GetStaticMagnitudeIfPossible(ActiveGE->Spec.GetLevel(), Duration);
			return Duration;
		}
	}
	
	return 0.f;
}

// 현재 스킬의 활성화(버프) 상태 체크 로직
bool USkillSlotBase::GetCurrentSkillActiveDuration(UAbilitySystemComponent* ASC, float& OutRemaining, float& OutTotal)
{
	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(CachedAbilitySpecHandle);
	if (!Spec || !Spec->Ability) return false;
	
	// 스킬 자체가 가진 태그(AbilityTags)를 기준으로 활성 효과 검색
	FGameplayTagContainer AbilityTags = Spec->Ability->AbilityTags;
	if (AbilityTags.Num() == 0) return false;

	FGameplayEffectQuery Query;
	Query.MakeQuery_MatchAnyOwningTags(AbilityTags);

	TArray<FActiveGameplayEffectHandle> ActiveHandles = ASC->GetActiveEffects(Query);

	float MaxDuration = 0.f;
	float MaxRemaining = 0.f;
	bool bFound = false;

	for (const FActiveGameplayEffectHandle& Handle : ActiveHandles)
	{
		const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(Handle);
		if (ActiveGE)
		{
			float Duration = ActiveGE->GetDuration();
			float Remaining = ActiveGE->GetTimeRemaining(ASC->GetWorld()->GetTimeSeconds());

			// 지속시간이 있고(즉발X, 무한X) 시간이 남은 경우만
			if (Duration > 0.f && Remaining > 0.f)
			{
				if (Remaining < MaxRemaining)
				{
					MaxRemaining = Remaining;
					MaxDuration = Duration;
					bFound = true;
				}
			}
		}
	}

	if (bFound)
	{
		OutRemaining = MaxRemaining;
		OutTotal = MaxDuration;
		return true;
	}

	return false;
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


