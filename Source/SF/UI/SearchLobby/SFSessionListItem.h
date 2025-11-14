#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "System/SFOSSGameInstance.h"
#include "SFSessionListItem.generated.h"

UCLASS(BlueprintType)
class SF_API USFSessionListItem : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	FSessionInfo Data; //세션 정보 전체 보관

	int32 SessionIndex; //리스트 내 세션 인덱스

	//====================================생성 함수====================================
	static USFSessionListItem* Make(UObject* Outer, const FSessionInfo& In, int32 Index)
	{
		USFSessionListItem* Item = NewObject<USFSessionListItem>(Outer);
		Item->Data = In;
		Item->SessionIndex = Index;
		return Item;
	}
	//=================================================================================
};
