#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "SFAbilitySystemComponent.generated.h"

struct FSFSavedAbilitySystemData;
DECLARE_MULTICAST_DELEGATE_TwoParams(FAbilityChangedDelegate, FGameplayAbilitySpecHandle, bool/*bGiven*/);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	
	// Sets default values for this component's properties
	USFAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	// 어빌리티 부여 시 AbilityChangedDelegate를 통해 UI 등에 변경사항 알림
	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	// 어빌리티 제거 시 AbilityChangedDelegate를 통해 UI 등에 변경사항 알림
	virtual void OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec) override;

	// InitAbilityActorInfo 호출 시 자동으로 활성화되어야 하는 어빌리티들을 처리
	void TryActivateAbilitiesOnSpawn();

	// 특정 입력 태그와 연결된 모든 어빌리티를 InputStartedSpecHandles에 추가
	void AbilityInputTagStarted(const FGameplayTag& InputTag);

	// 특정 입력 태그와 연결된 모든 어빌리티를 InputPressedSpecHandles와 InputHeldSpecHandles에 추가
	void AbilityInputTagPressed(const FGameplayTag& InputTag);

	// 특정 입력 태그와 연결된 모든 어빌리티를 InputReleasedSpecHandles에 추가하고 InputHeldSpecHandles에서 제거
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
	
	/**
	 * 어빌리티 입력 처리 (핵심 메서드)
	 * 매 프레임 호출되어 큐잉된 입력들을 실제 어빌리티 활성화로 변환
	 * 처리 순서:
	 * 1. InputStartedSpecHandles → AbilitySpecInputStarted 호출
	 * 2. InputPressedSpecHandles → 활성화된 어빌리티는 InputPressed 이벤트, 비활성화된 어빌리티는 활성화 시도
	 * 3. InputReleasedSpecHandles → AbilitySpecInputReleased 호출
	 * 4. AbilitiesToActivate → 실제 어빌리티 활성화 실행
	 * 5. 입력 핸들 배열들 초기화
	 */
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
	
	void ClearAbilityInput();

	/**
	 * 데이터 복원 시스템 관련 함수들
	 */

	// 모든 Attribute의 Base Value 저장
	void SaveAttributesToData(FSFSavedAbilitySystemData& OutData) const;

	// 저장된 Base Value로 Attribute 복원 
	void RestoreAttributesFromData(const FSFSavedAbilitySystemData& InData);

	// 현재 모든 어빌리티 저장 
	void SaveAbilitiesToData(FSFSavedAbilitySystemData& OutData) const;

	// 저장된 어빌리티 전체 복원
	void RestoreAbilitiesFromData(const FSFSavedAbilitySystemData& InData);

	// Duration GE 저장
	void SaveGameplayEffectsToData(FSFSavedAbilitySystemData& OutData) const;

	// Duration GE 복원
	void RestoreGameplayEffectsFromData(const FSFSavedAbilitySystemData& InData);
\
	/**
	 * 어빌리티 관리 헬퍼 함수
	 */
	
	void CancelActiveAbilities(const FGameplayTagContainer* WithTags = nullptr, const FGameplayTagContainer* WithoutTags = nullptr, UGameplayAbility* Ignore = nullptr, bool bIncludeOnSpawn = false);

protected:

	// ProcessAbilityInput에서 AbilitySecInputStarted 호출을 통해 GameCustom1 이벤트 발생
	virtual void AbilitySpecInputStarted(FGameplayAbilitySpec& Spec);
	
	// ProcessAbilityInput에서 활성화된 어빌리티는 InputPressed 이벤트 전달, 비활성화된 어빌리티는 활성화 시도
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;

	// ProcessAbilityInput에서 활성화된 어빌리티에 InputReleased 이벤트 전달
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;

public:
	 // 어빌리티가 추가되거나 제거될 때 UI 등에 알림을 보내는 델리게이트(OnGiveAbility/OnRemoveAbility에서 브로드캐스트됨)
	FAbilityChangedDelegate AbilityChangedDelegate;
	
protected:
	// 이번 프레임에 입력이 시작된 어빌리티들의 핸들
	TArray<FGameplayAbilitySpecHandle> InputStartedSpecHandles;
	
	// 이번 프레임에 입력이 눌린 어빌리티들의 핸들
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

	// 이번 프레임에 입력이 릴리즈된 어빌리티들의 핸들
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

	// 현재 입력이 지속적으로 눌려있는 어빌리티들의 핸들.
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;

	// 이 태그가 있으면 모든 어빌리티 입력 무시
	UPROPERTY(EditDefaultsOnly, Category = "SF|Input")
	FGameplayTagContainer InputBlockedTags;
};
