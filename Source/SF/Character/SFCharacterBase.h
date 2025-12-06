#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GameFramework/Character.h"
#include "MotionWarpingComponent.h"
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

	UFUNCTION(BlueprintCallable, Category = "SF|Character")
	UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }

	UFUNCTION(BlueprintPure, Category = "SF|Character")
	float GetGroundSpeed() const { return GroundSpeed; }

	UFUNCTION(BlueprintPure, Category = "SF|Character")
	float GetDirection() const { return Direction; }

	UFUNCTION(BlueprintPure, Category = "SF|Character")
	bool IsFalling() const;

	// 플레이어의 마지막 입력 의도 방향을 반환
	UFUNCTION(BlueprintPure, Category = "SF|Character")
	FVector GetLastInputDirection() const;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SF|Character|Animation")
	float GroundSpeed;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SF|Character|Animation")
	float Direction;

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;


private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFPawnExtensionComponent> PawnExtComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

private:
	void UpdateAnimValue();
};