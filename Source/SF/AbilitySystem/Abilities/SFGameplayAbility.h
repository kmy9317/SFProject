#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Character/Hero/Component/SFHeroMovementComponent.h"
#include "SFGameplayAbility.generated.h"

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

	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	USFAbilitySystemComponent* GetSFAbilitySystemComponentFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	ASFPlayerController* GetSFPlayerControllerFromActorInfo() const;
	
	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	ASFCharacterBase* GetSFCharacterFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	USFHeroMovementComponent* GetHeroMovementComponentFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	USFHeroComponent* GetHeroComponentFromActorInfo() const;
	
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
	
protected:
	//~UGameplayAbility interface
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	//~End of UGameplayAbility interface

protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|AbilityActivation")
	ESFAbilityActivationPolicy ActivationPolicy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SF|AbilityID")
	FName AbilityID;

	// Current camera mode set by the ability.
	TSubclassOf<USFCameraMode> ActiveCameraMode;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|Ability")
	TObjectPtr<UInputMappingContext> InputMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|AbilityInfo")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|AbilityInfo")
	FText Name;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|AbilityInfo")
	FText Description;

private:
	// 원본 모드 저장
	ESFSlidingMode SavedSlidingMode = ESFSlidingMode::Normal;

	// 적용 여부 (복원 필요 체크용)
	bool bSlidingModeApplied = false;
};
