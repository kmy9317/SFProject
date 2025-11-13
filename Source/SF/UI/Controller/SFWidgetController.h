#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SFWidgetController.generated.h"

class USFPrimarySet_Hero;
class USFCombatSet_Hero;
class UAbilitySystemComponent;
class UAttributeSet;

/**
 * 위젯 컨트롤러에 필요한 정보를 담을 구조체
 */
USTRUCT(BlueprintType)
struct FWidgetControllerParams
{
	GENERATED_BODY()
	
	FWidgetControllerParams() { }
	FWidgetControllerParams(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, USFPrimarySet_Hero* PrimaryAS, USFCombatSet_Hero* CombatAS, AGameStateBase* GS)
		: PlayerController(PC), PlayerState(PS), AbilitySystemComponent(ASC), PrimarySet(PrimaryAS), CombatSet(CombatAS), GameState(GS) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<APlayerController> PlayerController = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<APlayerState> PlayerState = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USFPrimarySet_Hero> PrimarySet = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USFCombatSet_Hero> CombatSet = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AGameStateBase> GameState = nullptr;
};

/**
 * 
 */
UCLASS()
class SF_API USFWidgetController : public UObject
{
	GENERATED_BODY()

public:
	// 위젯 컨트롤러 설정 함수
	UFUNCTION(BlueprintCallable, Category = "SF|Widget")
	void SetWidgetControllerParams(const FWidgetControllerParams& WCParams);

	//  위젯이 처음 생성될 때 초기값을 UI에 전달하기 위한 함수 (자식에서 구현)
	UFUNCTION(BlueprintCallable, Category="SF|Widget")
	virtual void BroadcastInitialValues() {}

	// Attribute 변경에 대한 콜백 함수들을 바인딩하는 함수 (자식에서 구현)
	UFUNCTION(BlueprintCallable, Category="SF|Widget")
	virtual void BindCallbacksToDependencies() {}

protected:
	
	UPROPERTY(BlueprintReadOnly, Category="SF|Model")
	TObjectPtr<APlayerController> PlayerController;

	UPROPERTY(BlueprintReadOnly, Category="SF|Model")
	TObjectPtr<APlayerState> PlayerState;

	UPROPERTY(BlueprintReadOnly, Category="SF|Model")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(BlueprintReadOnly, Category="SF|Model")
	TObjectPtr<USFPrimarySet_Hero> PrimarySet;
	
	UPROPERTY(BlueprintReadOnly, Category="SF|Model")
	TObjectPtr<USFCombatSet_Hero> CombatSet;

	UPROPERTY(BlueprintReadOnly, Category="SF|Model")
	TObjectPtr<AGameStateBase> GameState;
};
