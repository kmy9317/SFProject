#pragma once

#include "CoreMinimal.h"
#include "Controller/SFWidgetController.h"
#include "GameFramework/HUD.h"
#include "SFHUD.generated.h"

class USFPartyWidgetController;
struct FWidgetControllerParams;
class UAbilitySystemComponent;
class USFPrimarySet_Hero;
class USFCombatSet_Hero;
class USFUserWidget;
class USFWidgetController;
class USFOverlayWidgetController;

UCLASS()
class SF_API ASFHUD : public AHUD
{
	GENERATED_BODY()

public:
	// HUD 위젯 초기화
	void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, USFPrimarySet_Hero* PrimaryAS, USFCombatSet_Hero* CombatAS, AGameStateBase* GS);

	// Overlay 위젯 컨트롤러를 가져오는 함수
	UFUNCTION(BlueprintCallable, BlueprintPure)
	USFOverlayWidgetController* GetOverlayWidgetController(const FWidgetControllerParams& WCParams = FWidgetControllerParams());

	// Party 위젯 컨트롤러를 가져오는 함수
	UFUNCTION(BlueprintCallable, BlueprintPure)
	USFPartyWidgetController* GetPartyWidgetController(const FWidgetControllerParams& WCParams = FWidgetControllerParams());
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	USFWidgetController* GetWidgetController(TSubclassOf<USFWidgetController> ControllerClass, const FWidgetControllerParams& WCParams = FWidgetControllerParams());

	template<typename T>
	requires std::derived_from<T, USFWidgetController>
	T* GetWidgetController(const FWidgetControllerParams& WCParams)
	{
		return Cast<T>(GetWidgetController(T::StaticClass(), WCParams));
	}

private:
	// Overlay 위젯의 블루프린트 클래스
	UPROPERTY(EditAnywhere)
	TSubclassOf<USFUserWidget> OverlayWidgetClass;

	// Overlay 위젯 인스턴스
	UPROPERTY()
	TObjectPtr<USFUserWidget> OverlayWidget;

	// Overlay 위젯 컨트롤러의 블루프린트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "SF|Controller")
	TSubclassOf<USFOverlayWidgetController> OverlayWidgetControllerClass;
	
	// Party 위젯 컨트롤러의 블루프린트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "SF|Controller")
	TSubclassOf<USFPartyWidgetController> PartyWidgetControllerClass;
	
	UPROPERTY()
	TMap<TSubclassOf<USFWidgetController>, USFWidgetController*> WidgetControllers;
};
