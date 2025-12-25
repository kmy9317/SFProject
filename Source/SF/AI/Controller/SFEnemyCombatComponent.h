// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ControllerComponent.h"
#include "SFEnemyCombatComponent.generated.h"

class USFCombatSet_Enemy;
class USFAbilitySystemComponent;
struct FEnemyAbilitySelectContext;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFEnemyCombatComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	
	USFEnemyCombatComponent(const FObjectInitializer& ObjectInitializer);
	
	AActor* GetCurrentTarget() const { return CurrentTarget; }
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "SF|Combat")
	static USFEnemyCombatComponent* FindSFEnemyCombatComponent(const AController* Controller) 
	{ 
		return (Controller ? Controller->FindComponentByClass<USFEnemyCombatComponent>() : nullptr); 
	}
	
	// 초기화: ASC 및 AttributeSet 캐싱, Perception 설정 적용
	virtual void InitializeCombatComponent();

	// AIController에서 Perception 감지 시 호출할 함수
	void HandleTargetPerceptionUpdated(AActor* Actor, bool bSuccessfullySensed);

	// (기존 함수 유지) 어빌리티 선택 로직
	virtual bool SelectAbility(const FEnemyAbilitySelectContext& Context, const FGameplayTagContainer& SearchTags, FGameplayTag& OutSelectedTag);
	

protected:
	// 매 프레임 실행: 타겟과의 거리를 계산하고 태그 업데이트
	void UpdateCombatRangeTags();

	// 시야 관련 Attribute(SightRadius 등) 변경 시 Perception 설정 업데이트
	void UpdatePerceptionConfig();

	// ASC에 LooseTag 부여/제거
	void SetGameplayTagStatus(const FGameplayTag& Tag, bool bActive);

protected:
	// 캐싱된 데이터
	UPROPERTY()
	TObjectPtr<USFCombatSet_Enemy> CachedCombatSet;

	UPROPERTY()
	TObjectPtr<USFAbilitySystemComponent> CachedASC;
	
	// 현재 추적 중인 타겟
	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;
};
