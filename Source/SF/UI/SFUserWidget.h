// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFUserWidget.generated.h"

class USFWidgetController;
/**
 * 
 */
UCLASS()
class SF_API USFUserWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "SF|Widget")
	void SetWidgetController(USFWidgetController* InWidgetController);

	// WidgetController가 설정되었을 때 호출되는 함수로써 블루프린트에서 추가 로직 구현 가능
	UFUNCTION(BlueprintImplementableEvent, Category = "SF|Widget")
	void OnWidgetControllerSet();

	UFUNCTION(BlueprintPure, Category="SF|Widget")
	USFWidgetController* GetWidgetController() const { return WidgetController; }

	template<typename T>
	T* GetWidgetControllerTyped() const { return Cast<T>(WidgetController); }

protected:
	/// C++에서 오버라이드 가능한 WidgetController 설정 완료 콜백 
	virtual void NativeOnWidgetControllerSet() {}
	
	UPROPERTY(BlueprintReadOnly, Category = "SF|Widget")
	TObjectPtr<USFWidgetController> WidgetController;
};
