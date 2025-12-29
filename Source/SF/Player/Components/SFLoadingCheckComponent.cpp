#include "SFLoadingCheckComponent.h"

#include "SFSpectatorComponent.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Player/SFPlayerState.h"
#include "System/SFInitGameplayTags.h"


USFLoadingCheckComponent::USFLoadingCheckComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool USFLoadingCheckComponent::ShouldShowLoadingScreen(FString& OutReason) const
{
	AController* Controller = GetController<AController>();
	if (!Controller)
	{
		OutReason = TEXT("No Controller");
		return true;
	}

	// 죽은 플레이어는 로딩 스크린 불필요
	if (ASFPlayerState* SFPS = Controller->GetPlayerState<ASFPlayerState>())
	{
		if (SFPS->IsDead())
		{
			return false; 
		}
	}

	// 관전 모드 진입시 로딩 스크린 불필요
	if (USFSpectatorComponent* SpectatorComp = Controller->FindComponentByClass<USFSpectatorComponent>())
	{
		if (SpectatorComp->IsSpectating())
		{
			return false;
		}
	}
	
	if (APawn* OwnedPawn = GetPawn<APawn>())
	{
		if (const USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(OwnedPawn))
		{
			// Pawn의 초기화 상태가 'GameplayReady'에 도달했는지 확인
			// (GameFrameworkComponentManager를 사용하거나, 컴포넌트 내부 변수 확인)
			UGameFrameworkComponentManager* Manager = UGameFrameworkComponentManager::GetForActor(OwnedPawn);
            
			// 만약 Pawn이 아직 GameplayReady 상태가 아니라면 로딩 화면 유지
			if (Manager && !Manager->HasFeatureReachedInitState(OwnedPawn, USFPawnExtensionComponent::NAME_ActorFeatureName, SFGameplayTags::InitState_GameplayReady))
			{
				OutReason = TEXT("Waiting for Pawn Initialization (GameplayReady)...");
				return true; // 로딩 스크린 표시 유지
			}
			else
			{
				return false; // 로딩 스크린 표시 제거
			}
		}
	}

	OutReason = TEXT("Waiting for Pawn Initialization (GameplayReady)...");
	return true;
}



