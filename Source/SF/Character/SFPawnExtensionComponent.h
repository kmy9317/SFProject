// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"

#include "SFPawnExtensionComponent.generated.h"


class USFPawnData;
class USFAbilitySystemComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFPawnExtensionComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	USFPawnExtensionComponent(const FObjectInitializer& ObjectInitializer);

	/** FeatureName 정의 */
	static const FName NAME_ActorFeatureName;

	//~ Begin IGameFrameworkInitStateInterface interface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface interface

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "SF|Pawn")
	static USFPawnExtensionComponent* FindPawnExtensionComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<USFPawnExtensionComponent>() : nullptr); }

	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }

	void SetPawnData(const USFPawnData* InPawnData);

	/** Gets the current ability system component, which may be owned by a different actor */
	UFUNCTION(BlueprintPure, Category = "SF|Pawn")
	USFAbilitySystemComponent* GetSFAbilitySystemComponent() const { return AbilitySystemComponent; }

	/** AbilitySystemComponent의 AvatorActor 대상 초기화 or 해제 호출 */
	void InitializeAbilitySystem(USFAbilitySystemComponent* InASC, AActor* InOwnerActor);
	void UninitializeAbilitySystem();

	/** Should be called by the owning pawn when the pawn's controller changes. */
	void HandleControllerChanged();

	/** Should be called by the owning pawn when the player state has been replicated. */
	void HandlePlayerStateReplicated();

	/** Should be called by the owning pawn when the input component is setup. */
	void SetupPlayerInputComponent();

	/** OnAbilitySystem[Initialized|Uninitialized] Delegate에 추가: */
	void OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate);
	void OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate);

protected:

	/**
	* UPawnComponent interfaces
	*/
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_PawnData();

	/** ASC Init과 Uninit의 Delegate 추가 */
	FSimpleMulticastDelegate OnAbilitySystemInitialized;
	FSimpleMulticastDelegate OnAbilitySystemUninitialized;

	/**
	 * PawnData 캐싱
	 */
	UPROPERTY(EditInstanceOnly, ReplicatedUsing = OnRep_PawnData, Category = "SF|Pawn")
	TObjectPtr<const USFPawnData> PawnData;

	/** Pointer to the ability system component that is cached for convenience. */
	UPROPERTY(Transient)
	TObjectPtr<USFAbilitySystemComponent> AbilitySystemComponent;
};
