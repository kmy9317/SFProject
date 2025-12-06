#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SFGameplayAbility.generated.h"

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
	
	ESFAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }

	UFUNCTION(BlueprintCallable, Category = "SF|Ability")
	AController* GetControllerFromActorInfo() const;

	void TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const;

	FName GetAbilityID() const { return AbilityID; }
protected:
	//~UGameplayAbility interface
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	//~End of UGameplayAbility interface

protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|AbilityActivation")
	ESFAbilityActivationPolicy ActivationPolicy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SF|AbilityID")
	FName AbilityID;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|Ability")
	TObjectPtr<UInputMappingContext> InputMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|AbilityInfo")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|AbilityInfo")
	FText Name;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|AbilityInfo")
	FText Description;
};
