#include "CommonButtonBase.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/WidgetAnimation.h"


void UCommonButtonBase::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsValid(Text_Title))
	{
		Text_Title->SetText(ButtonTitle);
	}
}

void UCommonButtonBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(Btn_Clickable))
	{
		Btn_Clickable->OnHovered.AddDynamic(this, &UCommonButtonBase::OnButtonHovered);
		Btn_Clickable->OnUnhovered.AddDynamic(this, &UCommonButtonBase::OnButtonUnHovered);
		Btn_Clickable->OnClicked.AddDynamic(this, &UCommonButtonBase::OnButtonClicked);
	}
}

void UCommonButtonBase::OnButtonHovered()
{
	if (IsValid(HoverSound))
	{
		UGameplayStatics::PlaySound2D(this, HoverSound);
	}

	if (IsValid(Anim_HoverGlow))
	{
		PlayAnimation(Anim_HoverGlow, 0.0f, 0, EUMGSequencePlayMode::Forward);
	}
}

void UCommonButtonBase::OnButtonUnHovered()
{
	if (IsValid(Anim_HoverGlow))
	{
		StopAnimation(Anim_HoverGlow);
	}
}

void UCommonButtonBase::OnButtonClicked()
{
	if (IsValid(ClickSound))
	{
		UGameplayStatics::PlaySound2D(this, ClickSound);
	}

	OnButtonClickedDelegate.Broadcast();
}
