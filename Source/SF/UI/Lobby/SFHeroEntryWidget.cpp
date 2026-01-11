#include "SFHeroEntryWidget.h"

#include "Character/Hero/SFHeroDefinition.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

#include "GameModes/Lobby/SFLobbyGameState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"

void USFHeroEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	HeroDefinition = Cast<USFHeroDefinition>(ListItemObject);
	
	if (HeroDefinition)
	{
		if (HeroIcon)
		{
			HeroIcon->GetDynamicMaterial()->SetTextureParameterValue(IconTextureMatParamName, HeroDefinition->LoadIcon());
		}
       
		if (HeroNameText)
		{
			HeroNameText->SetText(FText::FromString(HeroDefinition->GetHeroDisplayName().ToUpper()));
		}
	}

	// 초기화 시점 자가 진단
	bool bIsMyPick = false;

	if (HeroDefinition)
	{
		UWorld* World = GetWorld();
		APlayerState* MyPlayerState = GetOwningPlayerState();
		
		if (World && MyPlayerState)
		{
			ASFLobbyGameState* LobbyGameState = World->GetGameState<ASFLobbyGameState>();
			if (LobbyGameState)
			{
				const TArray<FSFPlayerSelectionInfo>& PlayerSelections = LobbyGameState->GetPlayerSelections();
             
				for (const FSFPlayerSelectionInfo& Selection : PlayerSelections)
				{
					// 유효한 정보이고, 이 선택 정보의 주인이 자신의 PS인가?
					if (Selection.IsValid() && Selection.IsForPlayer(MyPlayerState))
					{
						// 선택한 영웅이 현재 이 위젯의 영웅과 같은가?
						if (Selection.GetHeroDefinition() == HeroDefinition)
						{
							bIsMyPick = true;
						}
						// 내 정보를 찾았으면 루프 종료 (다른 플레이어 정보는 볼 필요 없음)
						break;
					}
				}
			}
		}
	}
		SetSelected(bIsMyPick);
}

void USFHeroEntryWidget::SetSelected(bool bIsSelected)
{
		FLinearColor TargetColor = bIsSelected ? FLinearColor(1.0f, 0.8f, 0.0f, 1.0f) : FLinearColor::White;

		if (HeroIcon)
		{
			HeroIcon->SetColorAndOpacity(TargetColor);
		}
}
