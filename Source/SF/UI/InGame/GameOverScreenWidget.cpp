// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InGame/GameOverScreenWidget.h"
#include "GameFramework/PlayerController.h"

#include "System/SFInitGameplayTags.h"

void UGameOverScreenWidget::PlayGameOverDirection()
{
	if (Anim_GameOVer)
	{
		PlayAnimation(Anim_GameOVer);
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeUIOnly InputModeData;

		InputModeData.SetWidgetToFocus(this->TakeWidget());
		InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

		PC->SetInputMode(InputModeData);
		PC->bShowMouseCursor = true;
	}
}

void UGameOverScreenWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);

	if (Animation == Anim_GameOVer)
	{
		// TODO : 애니메이션이 끝난 후 버튼을 활성화하거나 로비로 이동하는 로직 요청
		
	}
}
