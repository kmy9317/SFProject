#include "SFHUD.h"

#include "SFLogChannels.h"
#include "SFUserWidget.h"
#include "Blueprint/UserWidget.h"
#include "Controller/SFOverlayWidgetController.h"
#include "Controller/Party/SFPartyWidgetController.h"

USFOverlayWidgetController* ASFHUD::GetOverlayWidgetController(const FWidgetControllerParams& WCParams)
{
	return Cast<USFOverlayWidgetController>(GetWidgetController(OverlayWidgetControllerClass, WCParams));
}

USFPartyWidgetController* ASFHUD::GetPartyWidgetController(const FWidgetControllerParams& WCParams)
{
	return Cast<USFPartyWidgetController>(GetWidgetController(PartyWidgetControllerClass, WCParams));
}

USFWidgetController* ASFHUD::GetWidgetController(TSubclassOf<USFWidgetController> ControllerClass, const FWidgetControllerParams& WCParams)
{
	if (!ControllerClass)
	{
		return nullptr;
	}

	if (USFWidgetController** CachedController = WidgetControllers.Find(ControllerClass))
	{
		return *CachedController;
	}
	
	USFWidgetController* NewController = NewObject<USFWidgetController>(this, ControllerClass);
	NewController->SetWidgetControllerParams(WCParams);
	NewController->BindCallbacksToDependencies();
    
	WidgetControllers.Add(ControllerClass, NewController);
	return NewController;
}

void ASFHUD::InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, USFPrimarySet_Hero* PrimaryAS, USFCombatSet_Hero* CombatAS, AGameStateBase* GS)
{
	if (!OverlayWidgetClass ||!OverlayWidgetControllerClass ||!PartyWidgetControllerClass)
	{
		UE_LOG(LogSF, Warning, TEXT("OverlayWidgetClass, OverlayWidgetControllerClass, or PartyWidgetControllerClass is not set in BP_HUD."));
		return;
	}

	// Overlay 위젯 생성
	OverlayWidget = CreateWidget<USFUserWidget>(GetWorld(), OverlayWidgetClass);

	// 위젯 컨트롤러 파라미터 구성
	const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, PrimaryAS, CombatAS, GS);
	
	// 오버레이 위젯 컨트롤러 생성
	USFOverlayWidgetController* OverlayWidgetController = Cast<USFOverlayWidgetController>(GetWidgetController(OverlayWidgetControllerClass, WidgetControllerParams));
	if (OverlayWidgetController)
	{
		// 위젯에 위젯 컨트롤러 설정
		// 해당 시점에 OnWidgetControllerSet 함수가 호출됨
		// SFUserWidget을 상속받는 Overlay 블루프린트 위젯에서 OnWidgetControllerSet 이벤트 호출 가능
		// 자식이 없는 Overlay 위젯의 경우는 여기서 Attribute 변경에 대한 델리게이트를 바인딩하여 UI 업데이트 가능
		// 자식이 있는 Overlay 위젯 블루프린트 내 자식으로 가지고 있는 SFUserWidget타입의 위젯에 접근하여 WidgetController를 설정하고 여기서 또 OnWidgetControllerSet 함수를 호출
		// 해당 위젯에서 Attribute 변경에 대한 델리게이트를 바인딩하여 UI 업데이트 가능(Ex Overlay위젯의 자식으로 가지고 있는 HealthBar 위젯)
		OverlayWidget->SetWidgetController(OverlayWidgetController);

		// 위젯 컨트롤러가 초기값을 브로드캐스트하도록 호출
		OverlayWidgetController->BroadcastInitialSets();
	}
	
	// 뷰포트에 위젯 추가
	OverlayWidget->AddToViewport();
}
