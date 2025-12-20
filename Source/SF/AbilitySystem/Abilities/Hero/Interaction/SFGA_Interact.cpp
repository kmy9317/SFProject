#include "SFGA_Interact.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/Tasks/SFAbilityTask_WaitInputStart.h"
#include "AbilitySystem/Tasks/Interaction/SFAbilityTask_GrantNearbyInteraction.h"
#include "AbilitySystem/Tasks/Interaction/SFAbilityTask_WaitForInteractableTraceHit.h"
#include "Character/SFCharacterBase.h"
#include "Character/SFCharacterGameplayTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Interaction/SFInteractionQuery.h"
#include "Messages/SFInteractionMessages.h"
#include "Messages/SFMessageGameplayTags.h"
#include "Physics/SFCollisionChannels.h"

USFGA_Interact::USFGA_Interact(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = ESFAbilityActivationPolicy::OnSpawn;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void USFGA_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 상호작용 쿼리 정보 설정
	FSFInteractionQuery InteractionQuery;
	InteractionQuery.RequestingAvatar = GetAvatarActorFromActorInfo();     
	InteractionQuery.RequestingController = GetControllerFromActorInfo();  

	// 1. 시선 방향 정밀 타겟팅 태스크 시작
	if (USFAbilityTask_WaitForInteractableTraceHit* TraceHitTask = USFAbilityTask_WaitForInteractableTraceHit::WaitForInteractableTraceHit(this, InteractionQuery, SF_TraceChannel_Interaction, MakeTargetLocationInfoFromOwnerActor(), InteractionTraceRange, InteractionTraceRate, bShowTraceDebug))
	{
		// 상호작용 가능한 객체가 변경될 때마다 UI 업데이트
		TraceHitTask->InteractableChanged.AddDynamic(this, &ThisClass::UpdateInteractions);
		TraceHitTask->ReadyForActivation();
	}

	// 2. 서버에서만 주변 객체 스캔 및 어빌리티 부여 태스크 시작
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	if (AbilitySystem && AbilitySystem->GetOwnerRole() == ROLE_Authority)
	{
		// 주변 상호작용 가능한 객체들에게 어빌리티 부여
		USFAbilityTask_GrantNearbyInteraction* GrantAbilityTask = USFAbilityTask_GrantNearbyInteraction::GrantAbilitiesForNearbyInteractables(this, InteractionScanRange, InteractionScanRate);
		GrantAbilityTask->ReadyForActivation();
	}

	// 3. 상호작용 입력 대기 시작
	WaitInputStart();
}

void USFGA_Interact::UpdateInteractions(const TArray<FSFInteractionInfo>& InteractionInfos)
{
	if (IsLocallyControlled())
	{
		if (UGameplayMessageSubsystem::HasInstance(this))
		{
			FSFInteractionMessage Message;
			Message.Instigator = GetAvatarActorFromActorInfo(); 
			Message.bShouldRefresh = true;
			// 현재 홀딩 상호작용 중이 아닐 때만 활성 상태 변경 허용
			Message.bSwitchActive = (GetAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(SFGameplayTags::Character_State_Interact) == false);
			// 첫 번째 상호작용 정보를 사용 (우선순위가 가장 높은 것)
			Message.InteractionInfo = InteractionInfos.Num() > 0 ? InteractionInfos[0] : FSFInteractionInfo();
			UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetAvatarActorFromActorInfo());
			MessageSystem.BroadcastMessage(SFGameplayTags::Message_Interaction_Notice, Message);
		}
	}
	// 현재 상호작용 정보 캐시 업데이트
	CurrentInteractionInfos = InteractionInfos;
}

void USFGA_Interact::TriggerInteraction()
{
	// 상호작용 가능한 객체가 없으면 무시
	if (CurrentInteractionInfos.Num() == 0)
	{
		return;
	}
	
	// 플레이어가 공중에 있는 경우 상호작용 불가
	ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(GetAvatarActorFromActorInfo());
	if (SFCharacter && SFCharacter->GetMovementComponent()->IsFalling())
	{
		return;
	}
	
	if (GetAbilitySystemComponentFromActorInfo())
	{
		// 첫 번째 상호작용 정보 사용 (우선순위가 가장 높은 것)
		const FSFInteractionInfo& InteractionInfo = CurrentInteractionInfos[0];
		
		AActor* Instigator = GetAvatarActorFromActorInfo();
		AActor* InteractableActor = nullptr;

		if (UObject* Object = InteractionInfo.Interactable.GetObject())
		{
			if (AActor* Actor = Cast<AActor>(Object))
			{
				// 객체가 액터인 경우 직접 사용
				InteractableActor = Actor;
			}
			else if (UActorComponent* ActorComponent = Cast<UActorComponent>(Object))
			{
				// 객체가 컴포넌트인 경우 소유자 액터 사용
				InteractableActor = ActorComponent->GetOwner();
			}
		}

		// AbilityInteract_Active 어빌리티를 EventData(상호작용 대상자, 상호작용되는 물체)정보를 넘겨서 trigger 되도록 함. 
		FGameplayEventData Payload;
		Payload.EventTag = SFGameplayTags::Ability_Interact_Active;
		Payload.Instigator = Instigator;                               
		Payload.Target = InteractableActor;
		// 게임플레이 이벤트 전송 (홀딩 어빌리티가 이 이벤트를 받아서 활성화됨)
		SendGameplayEvent(SFGameplayTags::Ability_Interact_Active, Payload);
	}
}

void USFGA_Interact::WaitInputStart()
{
	// 입력 대기 태스크 생성
	if (USFAbilityTask_WaitInputStart* InputStartTask = USFAbilityTask_WaitInputStart::WaitInputStart(this))
	{
		// 입력 감지 시 OnInputStart 콜백 함수 바인딩
		InputStartTask->OnStart.AddDynamic(this, &ThisClass::OnInputStart);
		InputStartTask->ReadyForActivation();
	}
}

void USFGA_Interact::OnInputStart()
{
	// 현재 타겟팅된 상호작용 실행
	TriggerInteraction();

	// 다음 입력을 위해 다시 입력 대기 상태로 전환
	WaitInputStart();
}


