#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/Hero/SFHeroAnimationData.h"
#include "Character/Hero/Component/SFHeroMovementComponent.h"
#include "StructUtils/InstancedStruct.h"
#include "SFGameplayAbility.generated.h"

class USFAbilityCost;
class ASFPlayerState;
class UInputAction;
class USFEquipmentComponent;
class USFHeroAnimationData;
class USFPawnExtensionComponent;
class ASFPlayerController;
class USFHeroComponent;
class USFCameraMode;
class UInputMappingContext;
class USFAbilitySystemComponent;
class ASFCharacterBase;

UENUM(BlueprintType)
enum class ESFAbilityActivationPolicy : uint8
{
	/** Input이 Trigger 되었을 경우 (Presssed/Released) */
	OnInputTriggered,
	/** Input이 Held되어 있을 경우 */
	WhileInputActive,
	/** avatar가 생성되었을 경우, 바로 할당(패시브 스킬 등) */
	OnSpawn,
	/** Event 발생시 활성화 되는 어빌리티 타입*/
	Manual 
};

UCLASS()
class SF_API USFGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual FInstancedStruct SaveCustomPersistentData() const 
	{ 
		return FInstancedStruct(); // 빈 구조체 (데이터 없음)
	}
	virtual void RestoreCustomPersistentData(const FInstancedStruct& Data) {}
	
	// 커스텀 세이브 데이터 유무
	virtual bool HasCustomPersistentData() const { return false; }

	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	USFAbilitySystemComponent* GetSFAbilitySystemComponentFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	ASFPlayerController* GetSFPlayerControllerFromActorInfo() const;

	UFUNCTION(BlueprintPure, Category = "SF|Ability")
	ASFPlayerState* GetSFPlayerStateFromActorInfo() const;
	
	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	ASFCharacterBase* GetSFCharacterFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	USFHeroMovementComponent* GetHeroMovementComponentFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	USFHeroComponent* GetHeroComponentFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "SF|Ability|Animation")
	USFHeroAnimationData* GetHeroAnimationData() const;

	UFUNCTION(BlueprintCallable, Category = "SF|Ability|Equipment")
	USFEquipmentComponent* GetEquipmentComponent() const;

	UFUNCTION(BlueprintCallable, Category = "SF|Ability|Equipment")
	FSFMontagePlayData GetMainHandEquipMontageData() const;
	
	ESFAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }

	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	AController* GetControllerFromActorInfo() const;

	void TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const;

	FName GetAbilityID() const { return AbilityID; }

	UFUNCTION(BlueprintCallable, Category = "SF|Ability|Team")
	ETeamAttitude::Type GetAttitudeTowards(AActor* Target) const;

	UFUNCTION(BlueprintCallable, Category = "SF|Ability|Camera")
	void SetCameraMode(TSubclassOf<USFCameraMode> CameraMode);

	UFUNCTION(BlueprintCallable, Category = "SF|Ability|Camera")
	void ClearCameraMode();

	UFUNCTION(BlueprintCallable, Category = "SF|Ability|Camera")
	void DisableCameraYawLimits();

	UFUNCTION(BlueprintCallable, Category = "SF|Ability|Camera")
	void DisableCameraYawLimitsForActiveMode();

	UFUNCTION(BlueprintCallable, Category = "SF|Ability|Movement")
	void ApplySlidingMode(ESFSlidingMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "SF|Ability|Movement")
	void RestoreSlidingMode();

	UFUNCTION(BlueprintCallable)
	void FlushPressedInput(UInputAction* InputAction);

	UFUNCTION(BlueprintCallable, Category = "SF|Ability|Animation")
	void ExecuteMontageGameplayCue(const FSFMontagePlayData& MontageData);

	UFUNCTION(BlueprintCallable, Category = "SF|Ability|Input")
	void RestorePlayerInput(bool bRestoreLookInput = false);

	UFUNCTION(BlueprintCallable, Category = "SF|Ability|Input")
	void DisablePlayerInput(bool bDisableLookInput = false);

	// 계산된 마나 코스트 반환
	UFUNCTION(BlueprintCallable, Category = "SF|Cost")
	virtual float GetCalculatedManaCost(UAbilitySystemComponent* ASC = nullptr, int32 InLevel = -1) const;

	bool ShouldPersistOnTravel() const { return bShouldPersistOnTravel; }
	
protected:
	//~UGameplayAbility interface
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	//~End of UGameplayAbility interface

	// 스킬이 발동될 때 실행되는 함수 (언리얼 BP상의 Event ActivateAbility와 동일)
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	virtual bool CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FGameplayTagContainer* OptionalRelevantTags) override;
	
	bool CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);
	
	float CalculateManaCostFromGameplayEffect(const UGameplayEffect* CostGE, UAbilitySystemComponent* ASC, int32 AbilityLevel) const;
	
protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|AbilityActivation")
	ESFAbilityActivationPolicy ActivationPolicy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SF|AbilityID")
	FName AbilityID;

	// Current camera mode set by the ability.
	TSubclassOf<USFCameraMode> ActiveCameraMode;

	// 추가 비용 (기본 GE Cost 외)
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Cost")
	TArray<TObjectPtr<USFAbilityCost>> AdditionalCosts;

	// Travel 시 저장할 어빌리티인지 여부 (동적 부여 어빌리티는 false)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Persistence")
	bool bShouldPersistOnTravel = true;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|Ability")
	TObjectPtr<UInputMappingContext> InputMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|AbilityInfo")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|AbilityInfo")
	FText Name;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|AbilityInfo")
	FText Description;

	// 지속시간(Duration) 지정 용도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Duration")
	TSubclassOf<UGameplayEffect> DurationGameplayEffectClass;

	// [옵션] 혹자동으로 켜지지 않도록 스킬 별도 구분 용도 토글
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Duration")
	bool bAutoApplyDurationEffect = true;


private:
	// 원본 모드 저장
	ESFSlidingMode SavedSlidingMode = ESFSlidingMode::Normal;

	// 적용 여부 (복원 필요 체크용)
	bool bSlidingModeApplied = false;

protected:
	// 쿨타임 초기화 패시브 처리
	void TryProcCooldownReset();
};
