#include "SFDeathScreenWidget.h"

#include "Player/SFPlayerState.h"
#include "Player/Components/SFSpectatorComponent.h"

void USFDeathScreenWidget::PlayDeathDirection()
{
	if (Anim_Death)
	{
		PlayAnimation(Anim_Death);
	}
	else
	{
		// 애니메이션 없으면 바로 완료 처리
		OnDeathAnimationFinished.Broadcast();
	}
}

void USFDeathScreenWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);

	if (Animation == Anim_Death)
	{
		OnDeathAnimationFinished.Broadcast();
	}
}
