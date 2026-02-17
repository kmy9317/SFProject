#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SFCombatLibrary.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class ASFCharacterBase;

/**
 * 범위 데미지 적용 시 필요한 파라미터를 묶은 구조체
 */
USTRUCT(BlueprintType)
struct FSFAreaDamageParams
{
	GENERATED_BODY()

	// 데미지를 가하는 ASC
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> SourceASC = nullptr;

	// 데미지의 원인 액터 (Instigator)
	UPROPERTY()
	TObjectPtr<AActor> SourceActor = nullptr;

	// EffectCauser (nullptr이면 SourceActor 사용)
	UPROPERTY()
	TObjectPtr<AActor> EffectCauser = nullptr;

	// 오버랩 중심점
	FVector Origin = FVector::ZeroVector;

	// 콜리전 형태 (Sphere, Capsule 등)
	FCollisionShape OverlapShape;

	// SetByCaller 데미지 수치
	float DamageAmount = 0.f;

	// SetByCaller에 사용할 태그
	FGameplayTag DamageSetByCallerTag;

	// 데미지 GE 클래스 (nullptr이면 GameData 폴백)
	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageGEClass = nullptr;

	// 디버프 GE 클래스 (nullptr이면 미적용)
	UPROPERTY()
	TSubclassOf<UGameplayEffect> DebuffGEClass = nullptr;

	// 오버랩에서 무시할 액터 목록
	UPROPERTY()
	TArray<AActor*> IgnoreActors;
};


/**
 * 전투 관련 공통 유틸리티
 */
UCLASS()
class SF_API USFCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	// 범위 내 적에게 GE 기반 데미지(+ 선택적 디버프) 적용(팀 필터링, 중복 액터 필터링, GE 폴백 로직을 내부에서 처리)
	static void ApplyAreaDamage(const FSFAreaDamageParams& Params);
	
	UFUNCTION(BlueprintPure, Category = "SF|Combat")
	static bool ShouldDamageTarget(AActor* SourceActor, AActor* TargetActor);
	
	static TSubclassOf<UGameplayEffect> ResolveDamageGEClass(TSubclassOf<UGameplayEffect> InClass);
};
