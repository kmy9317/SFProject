#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GameFramework/Character.h"
#include "SFCharacterBase.generated.h"

class USFPawnExtensionComponent;
class USFAbilitySystemComponent;

UCLASS()
class SF_API ASFCharacterBase : public ACharacter, public IAbilitySystemInterface, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASFCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnAbilitySystemInitialized();
	virtual void OnAbilitySystemUninitialized();

	/**
	 * IAbilitySystemInterface
	 */
	USFAbilitySystemComponent* GetSFAbilitySystemComponent() const;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;

	void ToggleCrouch();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;


private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFPawnExtensionComponent> PawnExtComponent;

};
