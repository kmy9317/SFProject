// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Common/RewardCardBase.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/Button.h"


void URewardCardBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Click)
	{
		Btn_Click->OnClicked.AddDynamic(this, &URewardCardBase::OnCardClicked); 
	}
	
}

void URewardCardBase::SetCardData(const FTempCardInfo& InData)
{
	if (Text_Title) Text_Title->SetText(InData.CardName);
	if (Text_Desc) Text_Desc->SetText(InData.Description);

	if (Image_Icon && InData.Icon)
	{
		Image_Icon->SetBrushFromTexture(InData.Icon);
	}

	if (Border_Frame && RarityColors.Contains(InData.Rarity))
	{
		Border_Frame->SetBrushColor(RarityColors[InData.Rarity]);
	}
}

void URewardCardBase::OnCardClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("%d 번 카드 선택!"), CurrentCardIndex);

	// 선택된 카드 데이터 전달 델리게이트 구현 예정
	//OnCardSelectedDelegate.Broadcast(CurrentCardIndex);
}
