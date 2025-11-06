// Fill out your copyright notice in the Description page of Project Settings.


#include "SFAbilitySystemComponent.h"

#include "Abilities/SFGameplayAbility.h"


USFAbilitySystemComponent::USFAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputStartedSpecHandles.Reset();
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

void USFAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
	check(ActorInfo);
	check(InOwnerActor);

	// 새로운 Pawn Avatar가 설정되었는지 확인
	const bool bHasNewPawnAvatar = Cast<APawn>(InAvatarActor) && (InAvatarActor != ActorInfo->AvatarActor);
	
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (bHasNewPawnAvatar)
	{
		// TODO : AnimInstance 초기화

		// 스폰 시 자동으로 활성화되어야 하는 어빌리티들 처리
		TryActivateAbilitiesOnSpawn();
	}
}

void USFAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);

	if (AbilityChangedDelegate.IsBound())
	{
		AbilityChangedDelegate.Broadcast(AbilitySpec.Handle, true);
	}
}

void USFAbilitySystemComponent::OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	if (AbilityChangedDelegate.IsBound())
	{
		AbilityChangedDelegate.Broadcast(AbilitySpec.Handle, false);
	}
	
	Super::OnRemoveAbility(AbilitySpec);
}

void USFAbilitySystemComponent::TryActivateAbilitiesOnSpawn()
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (const USFGameplayAbility* SFAbilityCDO = Cast<USFGameplayAbility>(AbilitySpec.Ability))
		{
			SFAbilityCDO->TryActivateAbilityOnSpawn(AbilityActorInfo.Get(), AbilitySpec);
		}
	}
}

void USFAbilitySystemComponent::AbilitySpecInputStarted(FGameplayAbilitySpec& Spec)
{
	if (Spec.IsActive())
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		FPredictionKey OriginalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::GameCustom1, Spec.Handle, OriginalPredictionKey);
	}
}

void USFAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	if (Spec.IsActive())
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		FPredictionKey OriginalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, OriginalPredictionKey);
	}
}

void USFAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	if (Spec.IsActive())
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		FPredictionKey OriginalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, OriginalPredictionKey);
	}
}

void USFAbilitySystemComponent::AbilityInputTagStarted(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			{
				InputStartedSpecHandles.AddUnique(AbilitySpec.Handle);
			}
		}
	}
}

void USFAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			{
				InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
			}
		}
	}
}

void USFAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			{
				InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.Remove(AbilitySpec.Handle);
			}
		}
	}
}

void USFAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	// 이번 프레임에 활성화할 어빌리티들을 저장할 정적 배열
	// 정적 변수로 선언하여 매 프레임마다 메모리 할당을 피함
	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

	//
	// 1단계: 지속 입력으로 활성화되는 어빌리티들 처리 (WhileInputActive 정책)
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability && !AbilitySpec->IsActive())
			{
				const USFGameplayAbility* SFAbilityCDO = Cast<USFGameplayAbility>(AbilitySpec->Ability);
				// 지속 입력으로 활성화되는 어빌리티들 처리 (WhileInputActive 정책)
				if (SFAbilityCDO && SFAbilityCDO->GetActivationPolicy() == ESFAbilityActivationPolicy::WhileInputActive)
				{
					AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
				}
			}
		}
	}

	//
	// 2단계: 입력 시작 이벤트 처리 (Started 이벤트 → GameCustom1 이벤트 발생)
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputStartedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				// 활성화된 어빌리티만 Started 이벤트를 전달
				if (AbilitySpec->IsActive())
				{
					// GameCustom1 복제 이벤트 발생 (UD1AbilityTask_WaitInputStart가 감지)
					AbilitySpecInputStarted(*AbilitySpec);
				}
			}
		}
	}

	// 입력 pressed 이벤트 처리 (Pressed 이벤트 → 활성화 vs 입력 이벤트 분기)
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				// 어빌리티 스펙의 InputPressed 플래그 설정
				AbilitySpec->InputPressed = true;

				if (AbilitySpec->IsActive()) // 핵심 분기점: 현재 활성화된 어빌리티인 경우
				{
					// 새로운 활성화 없이 입력 이벤트만 전달
					// InputPressed 복제 이벤트 발생 (UD1AbilityTask_WaitInputStart 등이 감지)
					AbilitySpecInputPressed(*AbilitySpec);
				}
				else // 어빌리티가 비활성화된 경우
				{
					// 새로운 어빌리티 활성화 시도
					const USFGameplayAbility* SFAbilityCDO = Cast<USFGameplayAbility>(AbilitySpec->Ability);

					// OnInputTriggered 정책인 어빌리티만 입력으로 활성화 가능
					if (SFAbilityCDO && SFAbilityCDO->GetActivationPolicy() == ESFAbilityActivationPolicy::OnInputTriggered)
					{
						AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
					}
				}
			}
		}
	}

	//
	// 4단계: 큐잉된 모든 어빌리티들 일괄 활성화
	// 지속 입력과 눌림 입력을 한 번에 처리하여 중복 활성화 방지
	// 지속 입력이 어빌리티를 활성화한 후 눌림 입력이 같은 어빌리티에 입력 이벤트를 보내는 것을 방지
	//
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		// 실제 어빌리티 활성화 시도
		// 여기서 CanActivateAbility 체크 등 어빌리티 활성화 체크 로직이 실행됨
		TryActivateAbility(AbilitySpecHandle);
	}

	//
	// 5단계: 입력 릴리즈 이벤트 처리 (Released 이벤트)
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				// Ability Spec의 InputPressed 플래그 해제
				AbilitySpec->InputPressed = false;

				// 활성화된 어빌리티만 Release 이벤트를 전달
				if (AbilitySpec->IsActive())
				{
					// InputReleased 복제 이벤트 발생 (UAbilityTask_WaitInputRelease 등이 감지)
					AbilitySpecInputReleased(*AbilitySpec);
				}
			}
		}
	}

	//
	// 6단계: 캐시된 입력 핸들들 초기화
	// 다음 프레임을 위해 모든 입력 상태 배열을 리셋
	// InputHeldSpecHandles는 지속적으로 유지되므로 초기화하지 않음
	//
	InputStartedSpecHandles.Reset();
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void USFAbilitySystemComponent::ClearAbilityInput()
{
	InputStartedSpecHandles.Reset();
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}


