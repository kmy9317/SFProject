#include "SFHUD.h"

#include "SFLogChannels.h"
#include "SFUserWidget.h"
#include "Blueprint/UserWidget.h"
#include "Common/SFGameOverStatsWidget.h"
#include "Controller/SFSharedOverlayWidgetController.h"
#include "Controller/SFOverlayWidgetController.h"
#include "Controller/Party/SFPartyWidgetController.h"
#include "Player/SFPlayerState.h"
#include "Player/Components/SFPlayerCombatStateComponent.h"
#include "Player/Components/SFSharedUIComponent.h"

USFSharedOverlayWidgetController* ASFHUD::GetSharedOverlayWidgetController(const FWidgetControllerParams& WCParams)
{
	return Cast<USFSharedOverlayWidgetController>(GetWidgetController(SharedOverlayWidgetControllerClass, WCParams));
}

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

void ASFHUD::BeginPlay()
{
	Super::BeginPlay();

	// HUD가 생성되었으므로 SharedUIComponent의 InitState 재시도
	if (APlayerController* PC = GetOwningPlayerController())
	{
		if (USFSharedUIComponent* SharedUIComp = PC->FindComponentByClass<USFSharedUIComponent>())
		{
			SharedUIComp->CheckDefaultInitialization();
		}
	}

	// GameOverStats 초기화 (Hidden 상태로 미리 생성)
	InitGameOverStats();
}

void ASFHUD::InitSharedOverlay(APlayerController* PC, APlayerState* PS, AGameStateBase* GS)
{
	if (SharedOverlayWidget)
	{
		return;
	}

	if (!SharedOverlayWidgetClass || !SharedOverlayWidgetControllerClass)
	{
		UE_LOG(LogSF, Warning, TEXT("ASFHUD::InitSharedOverlay - SharedOverlayWidgetClass or SharedOverlayWidgetControllerClass is not set"));
		return;
	}

	// SharedOverlay 위젯 생성
	SharedOverlayWidget = CreateWidget<USFUserWidget>(GetWorld(), SharedOverlayWidgetClass);
	if (!SharedOverlayWidget)
	{
		return;
	}

	// SharedOverlay WidgetController (ASC/AttributeSet 불필요)
	const FWidgetControllerParams WCParams(PC, PS, nullptr, nullptr, nullptr, GS);
	if (USFSharedOverlayWidgetController* SharedOverlayController = GetSharedOverlayWidgetController(WCParams))
	{
		SharedOverlayWidget->SetWidgetController(SharedOverlayController);
		SharedOverlayController->BroadcastInitialSets();
	}

	SharedOverlayWidget->AddToViewport(SharedOverlayZOrder);
}

void ASFHUD::InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, USFPrimarySet_Hero* PrimaryAS, USFCombatSet_Hero* CombatAS, AGameStateBase* GS)
{
	if (OverlayWidget)
	{
		return;
	}
	
	if (!OverlayWidgetClass ||!OverlayWidgetControllerClass)
	{
		UE_LOG(LogSF, Warning, TEXT("ASFHUD::InitOverlay - OverlayWidgetClass or OverlayWidgetControllerClass is not set"));
		return;
	}

	// Overlay 위젯 생성
	OverlayWidget = CreateWidget<USFUserWidget>(GetWorld(), OverlayWidgetClass);
	if (!OverlayWidget)
	{
		return;
	}
	
	const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, PrimaryAS, CombatAS, GS);
	if (USFOverlayWidgetController* OverlayWidgetController = GetOverlayWidgetController(WidgetControllerParams))
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

	// CombatInfo 바인딩
	if (ASFPlayerState* SFPS = Cast<ASFPlayerState>(PS))
	{
		if (USFPlayerCombatStateComponent* CombatComp = SFPS->FindComponentByClass<USFPlayerCombatStateComponent>())
		{
			CombatComp->OnCombatInfoChanged.AddDynamic(this, &ASFHUD::OnCombatInfoChanged);
            
			// 초기 상태 적용
			const FSFHeroCombatInfo& CombatInfo = CombatComp->GetCombatInfo();
			UpdateHeroOverlayVisibility(CombatInfo.bIsDead);
		}
	}
}

void ASFHUD::InitGameOverStats()
{
	if (GameOverStatsWidget || !GameOverStatsWidgetClass)
	{
		return;
	}

	GameOverStatsWidget = CreateWidget<USFGameOverStatsWidget>(GetOwningPlayerController(), GameOverStatsWidgetClass);
	if (GameOverStatsWidget)
	{
		GameOverStatsWidget->SetVisibility(ESlateVisibility::Collapsed);
		GameOverStatsWidget->AddToViewport(GameOverStatsZOrder);
	}
}

void ASFHUD::OnCombatInfoChanged(const FSFHeroCombatInfo& CombatInfo)
{
	UpdateHeroOverlayVisibility(CombatInfo.bIsDead);
}

void ASFHUD::UpdateHeroOverlayVisibility(bool bIsDead)
{
	if (!OverlayWidget)
	{
		return;
	}

	if (bIsDead)
	{
		OverlayWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		OverlayWidget->SetVisibility(ESlateVisibility::Visible);
	}
}
