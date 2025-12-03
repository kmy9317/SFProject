// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RewardCardBase.generated.h"

class UTextBlock;
class UImage;
class UBorder;
class UButton;

// 리워드 카드 선택시 선택된 카드 정보 전달 델리게이트 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRewardCardSelectedSignature, int32, CardIndex);

// =========================================================
// [임시 데이터 영역]

// 1. 임시 등급
UENUM(BlueprintType)
enum class ETempCardRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic
};

// 2. 임시 카드 정보 Struct
USTRUCT(BlueprintType)
struct FTempCardInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText CardName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETempCardRarity Rarity;
};

// =========================================================

UCLASS()
class SF_API URewardCardBase : public UUserWidget
{
	GENERATED_BODY()

// UI Components Binding
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Title;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Desc;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage>	Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage>	Image_Back;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder>	Border_Frame;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Click;
	

	// Design Settings
	// BP 디테일 패널에서 색상 지정 필요 (Enum 색상과 매치)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "UI|Common");
	TMap<ETempCardRarity, FLinearColor> RarityColors;

public:
	virtual void NativeConstruct() override;

	// 카드 데이터 연동 함수
	UFUNCTION(BlueprintCallable, Category = "UI|Function")
	void SetCardData(const FTempCardInfo& InData);

	// 카드 그래픽 애니메이션 함수 -> BP상에서 구현
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "UI|Animation")
	void PlayCardReveal();

	// 카드 정보 전달 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "UI|Event")
	FOnRewardCardSelectedSignature OnCardSelectedDelegate;

	// 몇 번째 카드인지 저장하는 변수
	UPROPERTY(BlueprintReadWrite, Category = "UI|Data")
	int32 CurrentCardIndex;
	
private:

	// 카드 선택시 호출 함수
	UFUNCTION()
	void OnCardClicked();
	
};
