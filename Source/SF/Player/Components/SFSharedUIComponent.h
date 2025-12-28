#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "SFSharedUIComponent.generated.h"

class USFPlayerCombatStateComponent;
struct FSFHeroCombatInfo;

/**
 * SharedOverlay UI를 위한 InitState 관리 컴포넌트
 * PlayerState와 GameState의 복제 완료를 보장한 후
 * HUD에 UI 생성 시점을 알려주는 역할
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFSharedUIComponent : public UControllerComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
    USFSharedUIComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    static const FName NAME_SharedUIFeature;

    //~ Begin IGameFrameworkInitStateInterface interface
    virtual FName GetFeatureName() const override { return NAME_SharedUIFeature; }
    virtual void CheckDefaultInitialization() override;
    virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
    virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
    //~ End IGameFrameworkInitStateInterface interface

protected:
    virtual void OnRegister() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
