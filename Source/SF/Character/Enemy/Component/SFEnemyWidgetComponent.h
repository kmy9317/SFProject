#pragma once
#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "GameplayEffectTypes.h" 
#include "SFEnemyWidgetComponent.generated.h"

class UCommonBarBase;

UCLASS()
class SF_API USFEnemyWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	USFEnemyWidgetComponent();
	
	void InitializeWidget();
	
	void MarkAsAttackedByLocalPlayer();

protected:
	virtual void InitWidget() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void OnHealthChanged(const FOnAttributeChangeData& OnAttributeChangeData);
    
	// 빌보드 및 거리 체크
	void FaceToCamera();
	void UpdateVisibility(float DeltaTime);
    
	// 표시/숨김 관리 (최적화 포함)
	void ShowHealthBar();
	void OnEngagementExpired(); // 숨기기
	void ResetEngagementTimer();

	void UpdateHealthPercent();

private:
	UPROPERTY()
	TObjectPtr<UCommonBarBase> EnemyWidget;

	// --- 설정 변수 ---
	UPROPERTY(EditAnywhere, Category = "SF|Widget")
	float WidgetVerticalOffset = 50.0f;

	UPROPERTY(EditAnywhere, Category = "SF|Widget")
	FLinearColor HealthBarColor = FLinearColor::Red; 
    
	UPROPERTY(EditAnywhere, Category = "SF|Widget")
	float VisibleDurationAfterHit = 3.f;

	UPROPERTY(EditAnywhere, Category = "SF|Widget")
	float MaxRenderDistance = 3000.f;

	UPROPERTY(EditAnywhere, Category = "SF|Widget")
	float HideOnAngle = 90.f;

	// --- 상태 변수 ---
	float CurrentVisibleTime = 0.f;
	bool bEngagedByLocalPlayer = false;
	float CachedMaxHealth = 0.f;
    
	FTimerHandle EngagementTimerHandle;
};