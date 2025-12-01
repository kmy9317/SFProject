#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SFPlayerController.generated.h"

class USFLoadingCheckComponent;
class ASFPlayerState;
class USFAbilitySystemComponent;

/**
 * 
 */
UCLASS()
class SF_API ASFPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASFPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AController interface
	virtual void BeginPlay() override;
	virtual void OnUnPossess() override;
	virtual void OnRep_PlayerState() override;
	//~End of AController interface

	UFUNCTION(BlueprintCallable, Category = "SF|PlayerController")
	ASFPlayerState* GetSFPlayerState() const;
	
	UFUNCTION(BlueprintCallable, Category = "SF|PlayerController")
	USFAbilitySystemComponent* GetSFAbilitySystemComponent() const;

protected:
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFLoadingCheckComponent> LoadingCheckComponent;
};
