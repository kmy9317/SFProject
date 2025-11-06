// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "SFState.generated.h"


// State -> Runtime Instance라고 생각하면 됨

class USFState;
class USFStateMachine;

UENUM(BlueprintType)
enum class EStateStatus : uint8
{
	None UMETA(DisplayName = "None"),      // 아직 아무 상태도 아님
	Begin UMETA(DisplayName = "Begin"),    // EnterState() 직후
	Running UMETA(DisplayName = "Running"),// State 실행 중
	Paused UMETA(DisplayName = "Paused"), // 멈춤 
	End UMETA(DisplayName = "End"),        // ExitState() 완료
};

// 인스턴스를 다룰수 있는 Handler 
USTRUCT(BlueprintType)
struct FSFStateHandle
{
	GENERATED_BODY()
public:
	FSFStateHandle()
		:Handle(INDEX_NONE)
	{
		
	}
	bool IsValid() const
	{
		return Handle != INDEX_NONE;
	}
	
	bool operator==(const FSFStateHandle& Other) const
	{
		return Handle == Other.Handle;
	}
	bool operator!=(const FSFStateHandle& Other) const
	{
		return Handle != Other.Handle;
	}
	void GenerateNewHandle();

private:
	UPROPERTY()
	int32 Handle;
	
};

USTRUCT(BlueprintType)
struct FSFStateSpec
{
    GENERATED_BODY()
    
    FSFStateSpec() = default;
    
    FSFStateSpec(TSubclassOf<USFState> InClass, FGameplayTag InTag)
        : StateClass(InClass)
        , StateTag(InTag)
    {
        Handle.GenerateNewHandle();
    	
    }
    
    bool IsActive() const ;
  
    bool IsValid() const;
 
	void EnsureInstance(UObject* Owner, AActor* OwnerActor);
	
    void Enter();
	
    void Exit();
	
    void Pause();
	
    void Resume();
	
    void Update(float DeltaTime);
	
    bool CanTransitionTo(TSubclassOf<USFState> ToStateClass) const;


	FGameplayTag GetStateTag() const { return StateTag; }
	FSFStateHandle GetHandle() const { return Handle; }
	EStateStatus GetStatus() const { return Status; }
	USFState* GetStateInstance() const { return StateInstance; }
	TSubclassOf<USFState> GetStateClass() const { return StateClass; }
	float GetActivationTime() const { return ActivationTime; }
	void SetActivationTime(float InTime) { ActivationTime = InTime; }
private:
	UPROPERTY()
	TSubclassOf<USFState> StateClass;
    
	UPROPERTY()
	EStateStatus Status = EStateStatus::None;
    
	UPROPERTY()
	FGameplayTag StateTag;
    
	UPROPERTY()
	FSFStateHandle Handle;
    
	UPROPERTY()
	TObjectPtr<USFState> StateInstance;

	UPROPERTY()
	float ActivationTime = 0.f;
	
};



UCLASS(Blueprintable, BlueprintType, Abstract)
class SF_API USFState : public UObject
{
    GENERATED_BODY()

public:
    
    //State에 진입할 때 호출 
    UFUNCTION(BlueprintNativeEvent, Category = "State")
    void OnEnter();
    virtual void OnEnter_Implementation();
    
    //State에서 나갈 때 
    UFUNCTION(BlueprintNativeEvent, Category = "State")
    void OnExit();
    virtual void OnExit_Implementation();
    
    //매 프레임 호출 (StateMachine의 Tick)
    UFUNCTION(BlueprintNativeEvent, Category = "State")
    void OnUpdate(float DeltaTime);
	
    virtual void OnUpdate_Implementation(float DeltaTime);


	//중간에 다른 Action 난입 시 
	UFUNCTION(BlueprintNativeEvent, Category = "State")
	void OnPause();

	virtual void OnPause_Implementation();

	//다시 Running
	UFUNCTION(BlueprintNativeEvent, Category = "State")
	void OnResume();
	
	virtual void OnResume_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "State")
	bool CanTransitionTo(TSubclassOf<USFState> ToStateClass) const;

	virtual bool CanTransitionTo_Implementation(TSubclassOf<USFState> ToStateClass) const;
	
	
    // State 초기화 
    virtual void Initialize(USFStateMachine* InStateMachine, AActor* InOwner);
	
    UFUNCTION(BlueprintPure, Category = "State")
    USFStateMachine* GetStateMachine() const { return StateMachine; }
    
    UFUNCTION(BlueprintPure, Category = "State")
    AActor* GetOwnerActor() const { return OwnerActor; }


protected:
    // OwnerComp 호출? 
    UPROPERTY(BlueprintReadOnly, Category = "State")
    TObjectPtr<USFStateMachine> StateMachine;
    
    //OwnerActor
    UPROPERTY(BlueprintReadOnly, Category = "State")
    TObjectPtr<AActor> OwnerActor;
	
	
};