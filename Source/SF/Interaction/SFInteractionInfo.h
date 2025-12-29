#pragma once

#include "Abilities/GameplayAbility.h"
#include "SFInteractionInfo.generated.h"

class ISFInteractable;

UENUM(BlueprintType)
enum class ESFInteractionType : uint8
{
	Instant,      // Duration 무시, 즉시 실행
	TimedHold,    // Duration 시간 후 성공 
	GaugeBased    // 외부 게이지가 100 도달 시 성공 (부활 등)
};

/**
 * 상호작용에 필요한 모든 정보를 담는 핵심 구조체
 * 상호작용 가능한 객체가 플레이어에게 제공하는 상호작용의 세부사항을 정의
 */
USTRUCT(BlueprintType)
struct FSFInteractionInfo
{
	GENERATED_BODY()

public:
	/** 이 상호작용 정보를 제공하는 상호작용 가능한 객체 참조 */
	UPROPERTY(BlueprintReadWrite)
	TScriptInterface<ISFInteractable> Interactable;

	/** 상호작용 UI에 표시될 제목 (예: "상자 열기", "소생 시키기") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
	FText Title;

	/** 상호작용 UI에 표시될 상세 설명 (예: "F키를 눌러 문을 엽니다") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
	FText Content;

	/**
	 * 상호작용 실행 방식
	 * - Instant: 즉시 실행
	 * - TimedHold: Duration 시간 동안 홀딩 후 성공
	 * - GaugeBased: 외부 게이지가 100 도달 시 성공 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
	ESFInteractionType InteractionType = ESFInteractionType::Instant;

	/**
	 * 홀딩 상호작용의 지속시간 (초 단위)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info|Duration", meta = (EditCondition = "InteractionType == ESFInteractionType::TimedHold"))
	float Duration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info|GameplayEvent", meta = (EditCondition = "InteractionType == ESFInteractionType::GaugeBased"))
	FGameplayTag GaugeBasedCompleteEventTag;

	/** 상호작용 실행 시 플레이어에게 부여될 어빌리티 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info|Ability")
	TSubclassOf<UGameplayAbility> AbilityToGrant;

	/** 홀딩 시작 시 재생할 몽타주 태그 (HeroAnimationData에서 조회) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info|Animaiton")
	FGameplayTag ActiveStartMontageTag;

	/** 상호작용 완료 시 재생할 몽타주 태그 (HeroAnimationData에서 조회) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info|Animation")
	FGameplayTag ActiveEndMontageTag;

	/** 홀딩 중 지속적으로 재생될 게임플레이 큐 태그 (사운드, 이펙트 등) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Categories="GameplayCue"))
	FGameplayTag ActiveLoopGameplayCueTag;

	/** 이 상호작용을 위한 커스텀 UI 위젯 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info|UI")
	TSoftClassPtr<UUserWidget> InteractionWidgetClass;

	/**
	 * 모든 멤버 변수를 비교하여 완전히 동일한 경우에만 true 반환
	 * 상호작용 정보 변화 감지에 사용
	 */
	FORCEINLINE bool operator==(const FSFInteractionInfo& Other) const
	{
		return Interactable == Other.Interactable &&
			Title.IdenticalTo(Other.Title) &&
			Content.IdenticalTo(Other.Content) &&
			InteractionType == Other.InteractionType &&
			Duration == Other.Duration &&
			GaugeBasedCompleteEventTag == Other.GaugeBasedCompleteEventTag &&
			AbilityToGrant == Other.AbilityToGrant &&
			ActiveStartMontageTag == Other.ActiveStartMontageTag &&
			ActiveEndMontageTag == Other.ActiveEndMontageTag &&
			ActiveLoopGameplayCueTag == Other.ActiveLoopGameplayCueTag &&
			InteractionWidgetClass == Other.InteractionWidgetClass;
	}

	FORCEINLINE bool operator!=(const FSFInteractionInfo& Other) const
	{
		return !operator==(Other);
	}

	/**
	 * 상호작용 정보 정렬을 위한 비교 연산자
	 * 상호작용 가능한 객체의 포인터 주소를 기준으로 정렬
	 * TArray::Sort() 함수에서 사용
	 */
	FORCEINLINE bool operator<(const FSFInteractionInfo& Other) const
	{
		return Interactable.GetInterface() < Other.Interactable.GetInterface();
	}
};