#include "SFGA_Hero_ToggleInventory.h"
#include "AbilitySystemComponent.h"
#include "UI/Inventory/SFInventoryScreenWidget.h"
#include "Player/SFPlayerController.h"
#include "Input/SFInputGameplayTags.h"


USFGA_Hero_ToggleInventory::USFGA_Hero_ToggleInventory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = ESFAbilityActivationPolicy::OnInputTriggered;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void USFGA_Hero_ToggleInventory::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ASFPlayerController* PC = GetSFPlayerControllerFromActorInfo();
	if (!PC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 위젯 생성
	if (InventoryScreenWidgetClass)
	{
		InventoryScreenWidget = CreateWidget<USFInventoryScreenWidget>(PC, InventoryScreenWidgetClass);
		if (InventoryScreenWidget)
		{
			InventoryScreenWidget->AddToViewport(InventoryScreenZOrder);
			InventoryScreenWidget->InitializeScreen();

			// 닫힘 이벤트 바인딩
			InventoryScreenWidget->OnScreenClosed.AddDynamic(this, &ThisClass::OnScreenClosed);

			// 마우스 커서 표시
			PC->SetShowMouseCursor(true);
			PC->SetInputMode(FInputModeUIOnly());
		}
	}

	// Toggle 입력 대기 (Tab 재입력 시 닫기)
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		InputEventHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(SFGameplayTags::InputTag_ToggleInventory).AddUObject(
			this, &ThisClass::OnToggleInputReceived);
	}
}

void USFGA_Hero_ToggleInventory::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 입력 이벤트 해제
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(SFGameplayTags::InputTag_ToggleInventory).Remove(InputEventHandle);
	}

	// 위젯 정리
	if (InventoryScreenWidget)
	{
		InventoryScreenWidget->OnScreenClosed.RemoveAll(this);
		InventoryScreenWidget->RemoveFromParent();
		InventoryScreenWidget = nullptr;
	}

	// 마우스 커서 숨기기
	if (ASFPlayerController* PC = GetSFPlayerControllerFromActorInfo())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGA_Hero_ToggleInventory::OnToggleInputReceived(const FGameplayEventData* Payload)
{
	// Tab 재입력 → 닫기
	K2_EndAbility();
}

void USFGA_Hero_ToggleInventory::OnScreenClosed()
{
	// 위젯에서 닫기 요청 (ESC 등)
	K2_EndAbility();
}