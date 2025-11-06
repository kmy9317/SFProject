#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "State/SFState.h"  
#include "SFStateMachine.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFStateMachine : public UActorComponent
{
    GENERATED_BODY()
    
public:
    USFStateMachine(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
    
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;
    
    
    //State를 등록하고 Handle 반환 
    UFUNCTION(BlueprintCallable, Category = "State Machine")
    FSFStateHandle RegisterState(TSubclassOf<USFState> StateClass, FGameplayTag StateTag = FGameplayTag());
    
    // Handle로 State 활성화 
    UFUNCTION(BlueprintCallable, Category = "State Machine")
    bool ActivateState(FSFStateHandle Handle);
    
    // Class로 State 활성화
    UFUNCTION(BlueprintCallable, Category = "State Machine")
    bool ActivateStateByClass(TSubclassOf<USFState> StateClass);
    
    //Tag로 State 활성화 (선택) 
    UFUNCTION(BlueprintCallable, Category = "State Machine")
    bool ActivateStateByTag(FGameplayTag StateTag);
    
    // 현재 State의 Handle 
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "State Machine")
    FSFStateHandle GetCurrentStateHandle() const { return CurrentStateHandle; }
    
    // 현재 State의 Class 
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "State Machine")
    TSubclassOf<USFState> GetCurrentStateClass() const;
    
    // 특정 State에 있는지 확인 
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "State Machine")
    bool IsInState(TSubclassOf<USFState> StateClass) const;
    
    // 현재 State에 머문 시간 
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "State Machine")
    float GetTimeInCurrentState() const;

    //State를 넣음
    UFUNCTION(BlueprintCallable, Category = "State Machine")
    bool PushState(FSFStateHandle Handle);

    //State 제거  
    UFUNCTION(BlueprintCallable, Category = "State Machine")
    bool PopState();
protected:
    virtual void BeginPlay() override;
    
private:
    
    // Handle로 Spec 찾기 
    FSFStateSpec* FindStateSpec(FSFStateHandle Handle);
    
    const FSFStateSpec* FindStateSpec(FSFStateHandle Handle) const;
    
    // Class로 Spec 찾기 
    FSFStateSpec* FindStateSpecByClass(TSubclassOf<USFState> StateClass);
    
    // Tag로 Spec 찾기 
    FSFStateSpec* FindStateSpecByTag(FGameplayTag StateTag);
    
    // State 전환 실행 
    bool TransitionToState(FSFStateHandle NewStateHandle);
    
    //State 진입 
    void EnterState(FSFStateSpec* Spec);
    
    // State 종료
    void ExitState(FSFStateSpec* Spec);


private:
    
    UPROPERTY()
    TArray<FSFStateSpec> RegisterStateSpecs;
    
    // 현재 활성화된 State 
    UPROPERTY()
    FSFStateHandle CurrentStateHandle;

    // 현재 활성화 중인 State 그러나 Array의 가장 마지막에 있는 State만 Running하고 나머지는 잠시 대기 ? 
    UPROPERTY()
    TArray<FSFStateSpec> ActiveStateSpecs;
    
    // 초기 StateClass -> 이건 아마 PawnData나 이런걸로 하지 않을까 ? 생각중 
    UPROPERTY(EditAnywhere, Category = "State Machine")
    TArray<TSubclassOf<USFState>> InitialStateClasses;
};