#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "UI/SFUserWidget.h"
#include "SkillSlotBase.generated.h"

class UImage;
class UTextBlock;
class UWidgetAnimation;
class USoundBase;

UCLASS()
class SF_API USkillSlotBase : public USFUserWidget
{
	GENERATED_BODY()

protected:
	
	virtual void NativeOnWidgetControllerSet() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void InitializeSlot();
	// 쿨타임 갱신용 (ProgressBar)
	void RefreshCooldown();
	// 지속시간 갱신용 (Border)
	void RefreshActiveDuration();

	float GetActiveCooldownDuration(UAbilitySystemComponent* ASC, UGameplayAbility* Ability);
	// 현재 사용중인 스킬로 인해 적용된 지속시간을 찾아내는 헬퍼 함수
	bool GetCurrentSkillActiveDuration(UAbilitySystemComponent* ASC, float& OutRemaining, float& OutTotal);

	UFUNCTION()
	void OnAbilityChanged(FGameplayAbilitySpecHandle AbilitySpecHandle, bool bGiven);

	UFUNCTION()
	void OnChainStateChanged(FGameplayAbilitySpecHandle AbilitySpecHandle, int32 ChainIndex);

private:
	void UpdateChainIcon(int32 ChainIndex);

	// GE 복제 기반 체인 인덱스 갱신
	void RefreshChainIndex(UAbilitySystemComponent* ASC);
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_SkillIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_CooldownCover;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_SkillBorder_Active;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CooldownCount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_KeyPrompt;

	// 소모량 표시 텍스트
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Cost;

	// 쿨타임 종료 시 재생할 위젯 애니메이션
	UPROPERTY(Transient , meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_CooldownFinished;

	// 쿨타임 종료 시 재생할 사운드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Sound")
	TObjectPtr<USoundBase> CooldownFinishedSound;

protected:
	UPROPERTY()
	bool bIsOnCooldown;

	UPROPERTY()
	float CooldownTotalDuration;

	UPROPERTY()
	float CooldownEndTime;
	
	// 이 슬롯이 표시할 어빌리티의 InputTag
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "SF|SkillSlot", meta = (Categories = "InputTag"))
	FGameplayTag SlotInputTag;

	// 키 프롬프트 텍스트
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "SF|SkillSlot")
	FText KeyPromptText;

private:
	FGameplayAbilitySpecHandle CachedAbilitySpecHandle;
	float CachedCooldownDuration = 0.f;

	int32 CachedChainIndex = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SkillBorderDMI;
};
