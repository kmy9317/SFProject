#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "SFBuffArea.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

//범위 기반 버프/디버프 장판 액터
UCLASS()
class SF_API ASFBuffArea : public AActor
{
	GENERATED_BODY()

public:
	ASFBuffArea();

	//Ability 쪽에서 소환 직후 Source ASC 주입
	void InitializeArea(UAbilitySystemComponent* InSourceASC);

protected:
	virtual void BeginPlay() override;

	//========================= Config =========================

	//장판 반지름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SF|Area")
	float Radius = 600.f; //범위

	//범위 체크 간격
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SF|Area")
	float TickInterval = 0.3f; //Tick 간격

	//지속 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SF|Area")
	float Duration = 6.f; //유지 시간

	//대상 필터용 ActorTag (예: Player, Enemy)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SF|Area")
	FName RequiredActorTag = "Player"; //타겟 태그

	//장판 이펙트를 재생할 GameplayCue 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SF|Area|FX")
	FGameplayTag AreaCueTag; //장판 GC 태그

	//범위 안에 적용할 GameplayEffect들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SF|Area|Effects")
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply; //적용할 GE 리스트

	//==========================================================

	//이 장판에서 Cue를 재생하기 위한 ASC (자기 자신에게 붙음)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SF|Area")
	UAbilitySystemComponent* AreaASC; //장판용 ASC

	//Ability 쪽에서 넘겨준 소스 ASC (Effect 컨텍스트용)
	UPROPERTY()
	UAbilitySystemComponent* SourceASC; //소스 ASC

	//대상별로 적용된 GE 핸들 목록
	//GC 경고 피하기 위해 WeakPtr 사용
	TMap<TWeakObjectPtr<AActor>, TArray<FActiveGameplayEffectHandle>> ActiveEffectMap; //대상별 GE 핸들

	FTimerHandle TickTimerHandle; //범위 체크 타이머
	FTimerHandle DurationTimerHandle; //지속 시간 타이머

protected:
	//TickInterval마다 실행되는 메인 로직
	void OnTickArea(); //범위 체크

	//Duration 끝났을 때 호출
	void OnDurationEnd(); //장판 종료

	//타겟 유효성 검사
	bool IsValidTarget(AActor* Target) const; //타겟 필터

	//한 대상에게 EffectsToApply 전부 Apply
	void ApplyEffectsTo(AActor* Target); //GE 적용

	//한 대상에게 이 장판이 Apply한 GE 전부 Remove
	void RemoveEffectsFrom(AActor* Target); //GE 제거

	//모든 GE 제거 + 타이머 정리 + Cue 제거
	void CleanupArea(); //전체 정리
};
