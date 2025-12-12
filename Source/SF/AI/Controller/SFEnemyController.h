#pragma once

#include "CoreMinimal.h"
#include "SFBaseAIController.h"
#include "DetourCrowdAIController.h"
#include "SFBaseAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "SFEnemyController.generated.h"

/**
 * Enemy AI Controller with Perception system and Crowd avoidance
 * Inherits common BT functionality from ASFBaseAIController
 */
UCLASS()
class SF_API ASFEnemyController : public ASFBaseAIController
{
	GENERATED_BODY()

public:
	ASFEnemyController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UFUNCTION(BlueprintCallable, Category = "AI|Combat")
	void SetTargetForce(AActor* NewTarget);

	// ISFAIControllerInterface
	virtual void InitializeAIController() override;

protected:
	virtual void Tick(float DeltaTime) override;

#pragma region Perception
	// 시야 감지 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	// 시야 감지 구조체
	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	// 감지 가능 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception")
	float SightRadius = 2000.f;

	// 감지 유지 종료 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception")
	float LoseSightRadius = 3500.f;

	// Idle 상태 시 시야각 (45도 = 정면 약 90도 부채꼴)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception")
	float PeripheralVisionAngleDegrees = 45.f;

	// 액터태그 기반 타겟 액터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception")
	FName TargetTag = FName("Player");

	// 시야 감지 이벤트 콜백 (감지/상실)
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// 시야 완전 소실 이벤트 (MaxAge 경과)
	UFUNCTION()
	void OnTargetPerceptionForgotten(AActor* Actor);
#pragma endregion

#pragma region Debug
	// 시야/범위 디버그 시각화 (콘솔: AI.ShowDebug 1)
	void DrawDebugPerception();
#pragma endregion
};