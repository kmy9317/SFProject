#include "SFLobbyWidget.h"

#include "SFHeroEntryWidget.h"
#include "Character/Hero/SFHeroDefinition.h"
#include "Components/Button.h"
#include "Components/TileView.h"
#include "Engine/StreamableManager.h"
#include "GameModes/Lobby/SFLobbyGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Lobby/SFLobbyPlayerController.h"
#include "Player/Lobby/SFLobbyPlayerState.h"
#include "System/SFAssetManager.h"
#include "UI/Common/CommonButtonBase.h"
#include "UI/Upgrade//SFPermanentUpgradeWidget.h"
#include "Components/Image.h"

void USFLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ConfigureGameState();
	SFLobbyPlayerController = GetOwningPlayer<ASFLobbyPlayerController>();
	SFLobbyPlayerState = GetOwningPlayerState<ASFLobbyPlayerState>();

	if (Button_Start)
	{
		Button_Start->OnButtonClickedDelegate.AddDynamic(this, &ThisClass::StartMatchButtonClicked);
		Button_Start->SetIsEnabled(false);
		Button_Start->SetVisibility(ESlateVisibility::Hidden);
	}

	if (Button_Ready)
	{
		Button_Ready->OnButtonClickedDelegate.AddDynamic(this, &ThisClass::ReadyButtonClicked);
		Button_Ready->SetIsEnabled(false);
	}

	if (Button_Upgrade)
	{
		Button_Upgrade->OnButtonClickedDelegate.AddDynamic(this, &ThisClass::UpgradeButtonClicked);
	}
	
	USFAssetManager::Get().LoadHeroDefinitions(FStreamableDelegate::CreateUObject(this, &ThisClass::HeroDefinitionLoaded));

	if (HeroSelectionTileView)
	{
		HeroSelectionTileView->OnItemSelectionChanged().AddUObject(this, &USFLobbyWidget::HeroSelected);
	}

	if (UKismetSystemLibrary::IsServer(GetWorld()))
	{
		Button_Start->SetVisibility(ESlateVisibility::Visible);
	}
}

void USFLobbyWidget::ConfigureGameState()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	SFGameState = World->GetGameState<ASFLobbyGameState>();
	if (!SFGameState)
	{
		World->GetTimerManager().SetTimer(ConfigureGameStateTimerHandle, this, &ThisClass::ConfigureGameState, 1.f);
	}
	else
	{
		SFGameState->OnPlayerSelectionUpdated.AddUObject(this, &ThisClass::UpdatePlayerSelectionDisplay);
		UpdatePlayerSelectionDisplay(SFGameState->GetPlayerSelections());
	}
}

void USFLobbyWidget::UpdatePlayerSelectionDisplay(const TArray<FSFPlayerSelectionInfo>& PlayerSelections)
{
	// 1. 선택 된 영웅 검색
	USFHeroDefinition* MySelectedHeroDef = nullptr;
	const FSFPlayerSelectionInfo* MySelection = FindMySelection(PlayerSelections);

	if (MySelection && MySelection->IsValid())
	{
		MySelectedHeroDef = MySelection->GetHeroDefinition();
	}
	
	// 2. 화면에 보이는 모든 영웅 엔트리 위젯을 순회하며 '내 영웅'인지 확인
	if (HeroSelectionTileView)
	{
		for (UUserWidget* HeroEntryAsWidget : HeroSelectionTileView->GetDisplayedEntryWidgets())
		{
			USFHeroEntryWidget* HeroEntryWidget = Cast<USFHeroEntryWidget>(HeroEntryAsWidget);
			if (!HeroEntryWidget)
			{
				continue;
			}
           
			const USFHeroDefinition* WidgetHeroDef = HeroEntryWidget->GetHeroDefinition();
           
			// 내 선택이 존재하고(nullptr 아님) && 이 위젯이 그 영웅이라면 -> 황금색
			// 그 외 모든 경우(내가 선택 안 함, 남이 선택함, 다른 영웅 등) ->흰색
			bool bIsMine = (MySelectedHeroDef != nullptr) && (WidgetHeroDef == MySelectedHeroDef);
           
			HeroEntryWidget->SetSelected(bIsMine);
		}
	}

	// 3. Ready 버튼 활성화 상태 업데이트
	UpdateReadyButtonEnabled(PlayerSelections);

	// 4. 내 플레이어의 선택 정보를 찾아 Image_Hero 갱신
	if (Image_Hero)
	{
		// 위에서 이미 MySelection을 찾았으므로 재사용 가능하지만, 기존 흐름 유지를 위해 로직 보존
		if (MySelection && MySelection->GetHeroDefinition())
		{
			USFHeroDefinition* SelectedHeroDef = MySelection->GetHeroDefinition();
            
			UTexture2D* IconTexture = SelectedHeroDef->LoadIcon(); 

			if (IconTexture)
			{
				Image_Hero->SetBrushFromTexture(IconTexture);
				Image_Hero->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				Image_Hero->SetVisibility(ESlateVisibility::Hidden); 
			}
		}
		else
		{
			Image_Hero->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void USFLobbyWidget::HeroDefinitionLoaded()
{
	TArray<USFHeroDefinition*> LoadedHeroDefinitions;
	if (USFAssetManager::Get().GetLoadedHeroDefinitions(LoadedHeroDefinitions))
	{
		HeroSelectionTileView->SetListItems(LoadedHeroDefinitions);

		if (SFGameState)
		{
			UpdatePlayerSelectionDisplay(SFGameState->GetPlayerSelections());
		}
	}
}

void USFLobbyWidget::HeroSelected(UObject* SelectedUObject)
{
	if (!SFLobbyPlayerState)
	{
		return;
	}

	if (USFHeroDefinition* HeroDefinition = Cast<USFHeroDefinition>(SelectedUObject))
	{
		SFLobbyPlayerState->Server_SetSelectedHeroDefinition(HeroDefinition);
	}
}

void USFLobbyWidget::UpdateReadyButtonEnabled(const TArray<FSFPlayerSelectionInfo>& PlayerSelections) const
{
	if (!Button_Ready)
	{
		return;
	}
    
	const FSFPlayerSelectionInfo* MySelection = FindMySelection(PlayerSelections);
	bool bHasHero = MySelection && MySelection->GetHeroDefinition() != nullptr;
	Button_Ready->SetIsEnabled(bHasHero);
}

void USFLobbyWidget::SetAllPlayersReady(bool bInAllPlayersReady)
{
	if (bAllPlayersReady != bInAllPlayersReady)
	{
		bAllPlayersReady = bInAllPlayersReady;
        
		if (Button_Start)
		{
			Button_Start->SetIsEnabled(bAllPlayersReady);
		}
	}
}

const FSFPlayerSelectionInfo* USFLobbyWidget::FindMySelection(const TArray<FSFPlayerSelectionInfo>& PlayerSelections) const
{
	if (!SFLobbyPlayerState)
	{
		return nullptr;
	}
	return PlayerSelections.FindByPredicate([this](const FSFPlayerSelectionInfo& Selection)
		{
			return Selection.IsForPlayer(SFLobbyPlayerState);
		});
}

bool USFLobbyWidget::HasSelectedHero() const
{
	if (!SFLobbyPlayerState)
	{
		return false;
	}
    
	const FSFPlayerSelectionInfo& Selection = SFLobbyPlayerState->GetPlayerSelection();
	return Selection.GetHeroDefinition() != nullptr;
}

void USFLobbyWidget::ReadyButtonClicked()
{
	if (SFLobbyPlayerController)
	{
		SFLobbyPlayerController->ToggleReady();
	}
}

void USFLobbyWidget::StartMatchButtonClicked()
{
	if (SFLobbyPlayerController)
	{
		SFLobbyPlayerController->Server_RequestStartMatch();
	}
}

void USFLobbyWidget::UpgradeButtonClicked()
{
	if (!UpgradeWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("UpgradeWidgetClass 가 USFLobbyWidget BP에서 설정되지 않았습니다."))
		return;
	}

	USFPermanentUpgradeWidget* UpgradeWidget = CreateWidget<USFPermanentUpgradeWidget>(this, UpgradeWidgetClass);
	if (UpgradeWidget)
	{
		UpgradeWidget->AddToViewport(100);
	}
}

