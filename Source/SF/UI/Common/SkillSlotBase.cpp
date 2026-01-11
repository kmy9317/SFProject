#include "UI/Common/SkillSlotBase.h"

#include "GameplayEffect.h"

#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Interface/SFChainedSkill.h"
#include "UI/Controller/SFOverlayWidgetController.h"

void USkillSlotBase::NativeOnWidgetControllerSet()
{
	if (Img_CooldownCover)
	{
		Img_CooldownCover->SetVisibility(ESlateVisibility::Collapsed);
       
		// 다이내믹 머티리얼 인스턴스 미리 생성 (성능 최적화)
		Img_CooldownCover->GetDynamicMaterial();
	}
	if (Text_CooldownCount)
	{
		Text_CooldownCount->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_Cost)
	{
		Text_Cost->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Img_SkillBorder_Active)
	{
		Img_SkillBorder_Active->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		if (UMaterialInstanceDynamic* DMI = Img_SkillBorder_Active->GetDynamicMaterial())
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
		RefreshCooldown();			// 쿨타임 갱신
		RefreshActiveDuration();    // 지속시간 갱신
		RefreshManaCost();          // 마나 소모량 갱신ㄴ
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

	if (Spec->Ability)
	{
		float Rem = Spec->Ability->GetCooldownTimeRemaining(ASC->AbilityActorInfo.Get());
		bIsOnCooldown = (Rem > 0.0f);
	}
	else
	{
		bIsOnCooldown = false;
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
		// 쿨타임이 남아있다면 상태를 true로 설정
		bIsOnCooldown = true;

		// 아이콘을 어둡게 만듦 (RGB를 0.3 정도로 낮춤, A는 1.0 유지)
		if (Img_SkillIcon)
		{
			Img_SkillIcon->SetColorAndOpacity(FLinearColor(0.3f, 0.3f, 0.3f, 1.0f));
		}
		
		float TotalDuration = GetActiveCooldownDuration(ASC, Spec->Ability);
		
		if (Text_CooldownCount)
		{
			if (CooldownRemaining > 1.0f)
			{
				// 쿨타임이 1초보다 많이 남았으면 정수로 올림 표시 (예: 1.1초 -> 2)
				Text_CooldownCount->SetText(FText::AsNumber(FMath::CeilToInt(CooldownRemaining)));
			}
			else
			{
				// 1초 이하일 때: 소수점 첫째 자리까지 표시 (예: 0.9, 0.4)
				FNumberFormattingOptions NumberFormat;
				NumberFormat.MinimumIntegralDigits = 1;	// 정수부 최소 1자리 (0.x 형태 유지)
				NumberFormat.MaximumFractionalDigits = 1;	// 소수부 최대 1자리
				NumberFormat.MinimumFractionalDigits = 1;	// 소수부 최소 1자리 (딱 떨어져도 .0 표시)

				Text_CooldownCount->SetText(FText::AsNumber(CooldownRemaining, &NumberFormat));
			}
			Text_CooldownCount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}

		if (Img_CooldownCover && TotalDuration > 0.f)
		{
			if (Img_CooldownCover->GetVisibility() == ESlateVisibility::Collapsed)
			{
				Img_CooldownCover->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}

			if (UMaterialInstanceDynamic* DMI = Img_CooldownCover->GetDynamicMaterial())
			{
				// 남은 시간 비율 (예: 0.5 = 절반 남음)
				float Percent = FMath::Clamp(CooldownRemaining / TotalDuration, 0.0f, 1.0f);
             
				// 머티리얼 그래프에 있는 파라미터 이름 "Percent"에 값을 쏘아줌
				DMI->SetScalarParameterValue(FName("Percent"), Percent);
			}
		}
	}
	else
	{
		// 쿨타임이 0 이하인데, 방금 전까지 쿨타임 중(bIsOnCooldown == true)이었다면? --> 쿨타임 종료 시점
		if (bIsOnCooldown)
		{
			bIsOnCooldown = false; // 상태 리셋

			if (Anim_CooldownFinished)
			{
				PlayAnimation(Anim_CooldownFinished);
			}
			if (CooldownFinishedSound)
			{
				UGameplayStatics::PlaySound2D(this, CooldownFinishedSound);
			}
			
		}
		// 아이콘을 원래 색(밝은 흰색)으로 복구
		if (Img_SkillIcon)
		{
			Img_SkillIcon->SetColorAndOpacity(FLinearColor::White);
		}
		
		if (Text_CooldownCount)
		{
			Text_CooldownCount->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (Img_CooldownCover)
		{
			Img_CooldownCover->SetVisibility(ESlateVisibility::Collapsed);
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

	RefreshChainIndex(ASC);
	
	// 2. 현재 지속시간(Remaining)과 전체시간(Total)을 받아올 변수
	float RemainingTime = 0.f;
	float TotalDuration = 0.f;

	// 3. 현재 스킬이 '활성화(Active)' 상태인지 체크
	bool bIsActive = GetCurrentSkillActiveDuration(ASC, RemainingTime, TotalDuration);

	if (Img_SkillBorder_Active)
	{
		// 상태 A: 스킬 발동 중 (시간이 남았음) -> 보여줌.
		if (bIsActive &&  TotalDuration > 0.f)
		{
			// (1) 지속시간 UI가 꺼져있다면 켬.
			if (Img_SkillBorder_Active->GetVisibility() != ESlateVisibility::SelfHitTestInvisible)
			{
				Img_SkillBorder_Active->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}

			// (2) 지속시간 머터리얼 값 갱신 (시간 줄어듬)
			if (UMaterialInstanceDynamic* DMI = Img_SkillBorder_Active->GetDynamicMaterial())
			{
				float PercentValue = FMath::Clamp(RemainingTime / TotalDuration, 0.0f, 1.0f);
				DMI->SetScalarParameterValue(FName("Percent"), PercentValue);
			}
		}
		// 상태 B: 스킬 꺼짐 (평상시) -> 숨김.
		else
		{
			// (3) 지속시간 UI가 켜져있다면 끔. (뒤에 있는 회색 테두리만 보이게 함)
			if (Img_SkillBorder_Active->GetVisibility() != ESlateVisibility::Collapsed)
			{
				Img_SkillBorder_Active->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
	
}

void USkillSlotBase::RefreshManaCost()
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
	
	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(CachedAbilitySpecHandle);
	if (!Spec || !Spec->Ability)
	{
		return;
	}

	const USFGameplayAbility* SFAbility = Cast<USFGameplayAbility>(Spec->Ability);
	if (!SFAbility)
	{
		return;
	}

	// (실제 마나 소모량 계산)
	float CurrentManaCost = SFAbility->GetCalculatedManaCost(ASC);

	// 6. 값 변화 체크 -> 값이 같으면 UI 갱신 안 함)
	if (FMath::IsNearlyEqual(CachedManaCost, CurrentManaCost))
	{
		return;
	}

	CachedManaCost = CurrentManaCost;
	
	if (Text_Cost)
	{
		if (CurrentManaCost > 0.f)
		{
			Text_Cost->SetText(FText::AsNumber(FMath::RoundToInt(CurrentManaCost)));
			Text_Cost->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			Text_Cost->SetVisibility(ESlateVisibility::Collapsed);
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
			return ActiveGE->GetDuration();
		}
	}
	
	return 0.f;
}

// 현재 스킬의 활성화(버프) 상태 체크 로직
bool USkillSlotBase::GetCurrentSkillActiveDuration(UAbilitySystemComponent* ASC, float& OutRemaining, float& OutTotal)
{
    FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(CachedAbilitySpecHandle);
    if (!Spec || !Spec->Ability) return false;

    // 1. 연계 스킬인 경우: ComboStateEffect의 남은 시간 반환
    if (const ISFChainedSkill* ChainedSkill = Cast<ISFChainedSkill>(Spec->Ability))
    {
        TSubclassOf<UGameplayEffect> ComboStateClass = ChainedSkill->GetComboStateEffectClass();
        if (ComboStateClass)
        {
            // ComboStateEffect가 적용되어 있는지 확인
            FGameplayEffectQuery Query;
            Query.EffectDefinition = ComboStateClass;
            
            TArray<FActiveGameplayEffectHandle> ActiveHandles = ASC->GetActiveEffects(Query);
            
            for (const FActiveGameplayEffectHandle& Handle : ActiveHandles)
            {
                const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(Handle);
                if (ActiveGE)
                {
                    float Duration = ActiveGE->GetDuration();
                    float Remaining = ActiveGE->GetTimeRemaining(ASC->GetWorld()->GetTimeSeconds());
                    
                    if (Duration > 0.f && Remaining > 0.f)
                    {
                        OutRemaining = Remaining;
                        OutTotal = Duration;
                        return true;
                    }
                }
            }
        }

    	return false;
    }

    // 2. 일반 스킬 로직 (쿨타임 제외 필터링 포함)

	TArray<FActiveGameplayEffectHandle> ActiveHandles = ASC->GetActiveEffects(FGameplayEffectQuery());

    float MaxDuration = 0.f;
    float MaxRemaining = 0.f;
    bool bFound = false;

	const UGameplayEffect* CooldownCDO = Spec->Ability->GetCooldownGameplayEffect();

    for (const FActiveGameplayEffectHandle& Handle : ActiveHandles)
    {
        const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(Handle);

    	if (!ActiveGE) continue;

    	//  현재 스킬에 등록된 쿨타임 클래스와 같은지 비교
    	if (CooldownCDO && ActiveGE->Spec.Def && (ActiveGE->Spec.Def->GetClass() == CooldownCDO->GetClass()))
    	{
    		continue;	// 같을 경우 UI 업데이트 X
    	}

    	const UGameplayAbility* SourceAbility = ActiveGE->Spec.GetEffectContext().GetAbility();

    	if (SourceAbility && SourceAbility->GetClass() == Spec->Ability->GetClass())
    	{
    		float Duration = ActiveGE->GetDuration();
    		float Remaining = ActiveGE->GetTimeRemaining(ASC->GetWorld()->GetTimeSeconds());

    		if (Duration > 0.f && Remaining > 0.f)
    		{
    			if (Remaining > MaxRemaining)
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

void USkillSlotBase::RefreshChainIndex(UAbilitySystemComponent* ASC)
{
	if (!ASC || !CachedAbilitySpecHandle.IsValid())
	{
		return;
	}

	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(CachedAbilitySpecHandle);
	if (!Spec || !Spec->Ability)
	{
		return;
	}

	const ISFChainedSkill* ChainedSkill = Cast<ISFChainedSkill>(Spec->Ability);
	if (!ChainedSkill)
	{
		return;
	}

	TSubclassOf<UGameplayEffect> ComboStateClass = ChainedSkill->GetComboStateEffectClass();
	if (!ComboStateClass)
	{
		return;
	}

	// GE 카운트로 현재 체인 인덱스 계산
	int32 CurrentChain = ASC->GetGameplayEffectCount(ComboStateClass, nullptr);

	// 변경됐을 때만 아이콘 갱신
	if (CachedChainIndex != CurrentChain)
	{
		CachedChainIndex = CurrentChain;
		UpdateChainIcon(CurrentChain);
	}
}


