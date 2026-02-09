#include "SFHeroComponent.h"

#include "EnhancedInputSubsystems.h"
#include "Camera/SFCameraComponent.h"
#include "SFHero.h"
#include "System/SFInitGameplayTags.h"
#include "SFLogChannels.h"
#include "Character/SFPawnData.h"
#include "Character/SFPawnExtensionComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Input/SFInputComponent.h"
#include "Input/SFInputConfig.h"
#include "Input/SFInputGameplayTags.h"
#include "Player/SFPlayerController.h"
#include "Player/SFPlayerState.h"
#include "UI/SFHUD.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "AbilitySystem/Attributes/Hero/SFCombatSet_Hero.h"
#include "AbilitySystem/Attributes/Hero/SFPrimarySet_Hero.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Camera/SFCameraMode.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "System/Data/SFSettingsSaveGame.h"

const FName USFHeroComponent::NAME_ActorFeatureName("Hero");

USFHeroComponent::USFHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilityCameraMode = nullptr;
}

void USFHeroComponent::OnRegister()
{
	Super::OnRegister();

	if (!GetPawn<APawn>())
	{
		UE_LOG(LogSF, Error, TEXT("[USFHeroComponent::OnRegister] This component has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint."));
	}
	else
	{
		// Register with the init state system early, this will only work if this is a game world
		RegisterInitStateFeature();
	}
}

bool USFHeroComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	if (!CurrentState.IsValid() && DesiredState == SFGameplayTags::InitState_Spawned)
	{
		// As long as we have a real pawn, let us transition
		if (Pawn)
		{
			return true;
		}
	}
	else if (CurrentState == SFGameplayTags::InitState_Spawned && DesiredState == SFGameplayTags::InitState_DataAvailable)
	{
		// The player state is required.
		if (!GetPlayerState<ASFPlayerState>())
		{
			return false;
		}

		// If we're authority or autonomous, we need to wait for a controller with registered ownership of the player state.
		if (Pawn->GetLocalRole() != ROLE_SimulatedProxy)
		{
			AController* Controller = GetController<AController>();

			const bool bHasControllerPairedWithPS = (Controller != nullptr) && \
				(Controller->PlayerState != nullptr) && \
				(Controller->PlayerState->GetOwner() == Controller);

			if (!bHasControllerPairedWithPS)
			{
				return false;
			}
		}

		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		const bool bIsBot = Pawn->IsBotControlled();

		if (bIsLocallyControlled && !bIsBot)
		{
			ASFPlayerController* SFPC = GetController<ASFPlayerController>();

			// The input component and local player is required when locally controlled.
			if (!Pawn->InputComponent || !SFPC || !SFPC->GetLocalPlayer())
			{
				return false;
			}
		}

		return true;
	}
	else if (CurrentState == SFGameplayTags::InitState_DataAvailable && DesiredState == SFGameplayTags::InitState_DataInitialized)
	{
		// Wait for player state and extension component
		ASFPlayerState* SFPS = GetPlayerState<ASFPlayerState>();

		return SFPS && Manager->HasFeatureReachedInitState(Pawn, USFPawnExtensionComponent::NAME_ActorFeatureName, SFGameplayTags::InitState_DataInitialized);
	}
	else if (CurrentState == SFGameplayTags::InitState_DataInitialized && DesiredState == SFGameplayTags::InitState_GameplayReady)
	{
		// TODO add ability initialization checks?
		return true;
	}

	return false;
}

void USFHeroComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (CurrentState == SFGameplayTags::InitState_DataAvailable && DesiredState == SFGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		ASFPlayerState* SFPS = GetPlayerState<ASFPlayerState>();
		if (!ensure(Pawn && SFPS))
		{
			return;
		}

		const USFPawnData* PawnData = nullptr;

		if (USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnData = PawnExtComp->GetPawnData<USFPawnData>();

			// The player state holds the persistent data for this player (state that persists across deaths and multiple pawns).
			// The ability system component and attribute sets live on the player state.
			PawnExtComp->InitializeAbilitySystem(SFPS->GetSFAbilitySystemComponent(), SFPS);
		}

		if (ASFPlayerController* SFPC = GetController<ASFPlayerController>())
		{
			if (Pawn->InputComponent != nullptr)
			{
				InitializePlayerInput(Pawn->InputComponent);
			}

			if (SFPC->IsLocalController())
			{
				InitializeHUD();
			}
		}
		
		// Hook up the delegate for all pawns, in case we spectate later
		if (PawnData)
		{
			if (USFCameraComponent* CameraComponent = USFCameraComponent::FindCameraComponent(Pawn))
			{
				CameraComponent->DetermineCameraModeDelegate.BindDynamic(this, &ThisClass::DetermineCameraMode);
			}
		}
	}

	if (CurrentState == SFGameplayTags::InitState_DataInitialized && DesiredState == SFGameplayTags::InitState_GameplayReady)
	{
		
	}
}

void USFHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == USFPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == SFGameplayTags::InitState_DataInitialized)
		{
			// SFPawnExtensionComponent의 DataInitialized 상태 변화 관찰 후, SFHeroComponent도 DataInitialized 상태로 변경
			// - CanChangeInitState 확인
			CheckDefaultInitialization();
		}
	}
}

void USFHeroComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = { SFGameplayTags::InitState_Spawned, SFGameplayTags::InitState_DataAvailable, SFGameplayTags::InitState_DataInitialized, SFGameplayTags::InitState_GameplayReady };

	ContinueInitStateChain(StateChain);
}

void USFHeroComponent::BeginPlay()
{
	Super::BeginPlay();

	// [카메라 민감도 UI 에서 추가] 캐릭터가 태어날 때, 저장된 세이브 파일이 있다면 민감도를 불러와서 적용한
	if (USFSettingsSaveGame* LoadedData = Cast<USFSettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("Settings"), 0)))
	{
		SetMouseSensitivity(LoadedData->MouseSensitivity);
		UE_LOG(LogTemp, Log, TEXT("[SFHeroComponent] Loaded Sensitivity: %f"), LoadedData->MouseSensitivity);
	}
	else
	{
		// 세이브 파일이 없다면 기본값(0.5) 유지
		SetMouseSensitivity(0.5f);
	}

	// PawnExtensionComponent에 대해서 (PawnExtension Feature) OnActorInitStateChanged() 관찰하도록 (Observing)
	BindOnActorInitStateChanged(USFPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	// Notifies that we are done spawning, then try the rest of initialization
	ensure(TryToChangeInitState(SFGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void USFHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void USFHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = PC->GetLocalPlayer();
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	Subsystem->ClearAllMappings();

	if (const USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const USFPawnData* PawnData = PawnExtComp->GetPawnData<USFPawnData>())
		{
			if (const USFInputConfig* InputConfig = PawnData->InputConfig)
			{
				for (const FSFInputMappingContextAndPriority& Mapping : InputConfig->InputMappingContexts)
				{
					if (UInputMappingContext* IMC = Mapping.MappingContexts)
					{
						if (UEnhancedInputUserSettings* Settings = Subsystem->GetUserSettings())
						{
							Settings->RegisterInputMappingContext(IMC);
						}
						FModifyContextOptions Options = {};
						Options.bIgnoreAllPressedKeysUntilRelease = false;
						Subsystem->AddMappingContext(IMC, Mapping.Priority, Options);
					}
				}
				
				USFInputComponent* SFIC = Cast<USFInputComponent>(PlayerInputComponent);
				if (ensureMsgf(SFIC, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to USFInputComponent or a subclass of it.")))
				{
					TArray<uint32> BindHandles;
					SFIC->BindAbilityActions(InputConfig, this,&ThisClass::Input_AbilityInputTagStarted, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

					SFIC->BindNativeAction(InputConfig, SFGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move, /*bLogIfNotFound=*/ false);
					SFIC->BindNativeAction(InputConfig, SFGameplayTags::InputTag_Move, ETriggerEvent::Completed, this, &ThisClass::Input_MoveCompleted, /*bLogIfNotFound=*/ false);
					SFIC->BindNativeAction(InputConfig, SFGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse, /*bLogIfNotFound=*/ false);
					SFIC->BindNativeAction(InputConfig, SFGameplayTags::InputTag_Crouch, ETriggerEvent::Triggered, this, &ThisClass::Input_Crouch, /*bLogIfNotFound=*/ false);
					SFIC->BindNativeAction(InputConfig, SFGameplayTags::InputTag_UseQuickbar_1, ETriggerEvent::Started, this, &ThisClass::Input_UseQuickbar_1, false);
					SFIC->BindNativeAction(InputConfig, SFGameplayTags::InputTag_UseQuickbar_2, ETriggerEvent::Started, this, &ThisClass::Input_UseQuickbar_2, false);
					SFIC->BindNativeAction(InputConfig, SFGameplayTags::InputTag_UseQuickbar_3, ETriggerEvent::Started, this, &ThisClass::Input_UseQuickbar_3, false);
					SFIC->BindNativeAction(InputConfig, SFGameplayTags::InputTag_UseQuickbar_4, ETriggerEvent::Started, this, &ThisClass::Input_UseQuickbar_4, false);
				}
			}
		}
	}
}

void USFHeroComponent::Input_AbilityInputTagStarted(FGameplayTag InputTag)
{
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (const USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			if (USFAbilitySystemComponent* SFASC = PawnExtComp->GetSFAbilitySystemComponent())
			{
				SFASC->AbilityInputTagStarted(InputTag);
			}
		}
	}
}

void USFHeroComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (const USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			if (USFAbilitySystemComponent* SFASC = PawnExtComp->GetSFAbilitySystemComponent())
			{
				SFASC->AbilityInputTagPressed(InputTag);
			}
		}	
	}
}

void USFHeroComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	if (const USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (USFAbilitySystemComponent* SFASC = PawnExtComp->GetSFAbilitySystemComponent())
		{
			SFASC->AbilityInputTagReleased(InputTag);
		}
	}
}

void USFHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}

	AController* Controller = Pawn->GetController();
	FVector WorldDirection = FVector::ZeroVector;

	
	if (Controller)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector RightDir = MovementRotation.RotateVector(FVector::RightVector);
			WorldDirection += RightDir * Value.X;
		}

		if (Value.Y != 0.0f)
		{
			const FVector ForwardDir = MovementRotation.RotateVector(FVector::ForwardVector);
			WorldDirection += ForwardDir * Value.Y;
		}
	}

	// 로컬 플레이어의 입력 의도 방향 계산
	LastInputDirection = WorldDirection.GetSafeNormal();
	
	// if (const USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	// {
	// 	if (const USFAbilitySystemComponent* SFASC = PawnExtComp->GetSFAbilitySystemComponent())
	// 	{
	// 		// "Character.State.Attacking" 태그를 가지고 있다면?
	// 		// (GA_Hero_ComboAttack의 Activation Owned Tag에 이 태그가 있어야 함)
	// 		if (SFASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Attacking)) 
	// 		{
	// 			return; // 입력을 처리하지 않고 함수를 강제 종료 -> 이동 불가
	// 		}
	// 	}
	// }

	if (WorldDirection.IsNearlyZero() == false)
	{
		Pawn->AddMovementInput(WorldDirection, 1.0f);
	}
}

void USFHeroComponent::Input_MoveCompleted(const FInputActionValue& InputActionValue)
{
	LastInputDirection = FVector::ZeroVector;
}

void USFHeroComponent::Input_LookMouse(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}
	
	const FVector2D Value = InputActionValue.Get<FVector2D>();

	// [UI 마우스 민감도 부분에서 수정] X축(좌우) 입력에 민감도(MouseSensitivity)를 곱함
	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X * MouseSensitivity);
	}

	// [UI 마우스 민감도 부분에서 수정] Y축(상하) 입력에 민감도(MouseSensitivity)를 곱함
	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y * MouseSensitivity);
	}
}

void USFHeroComponent::Input_Crouch(const FInputActionValue& InputActionValue)
{
	// TODO : SFCharacterBase 참조 로직들 SFHero로 변경하면 좋아보임
	if (ASFCharacterBase* Character = GetPawn<ASFCharacterBase>())
	{
		Character->ToggleCrouch();
	}
}

void USFHeroComponent::Input_UseQuickbar_1(const FInputActionValue& InputActionValue)
{
	SendUseQuickbarEvent(0);
}

void USFHeroComponent::Input_UseQuickbar_2(const FInputActionValue& InputActionValue)
{
	SendUseQuickbarEvent(1);
}

void USFHeroComponent::Input_UseQuickbar_3(const FInputActionValue& InputActionValue)
{
	SendUseQuickbarEvent(2);
}

void USFHeroComponent::Input_UseQuickbar_4(const FInputActionValue& InputActionValue)
{
	SendUseQuickbarEvent(3);
}

void USFHeroComponent::SendUseQuickbarEvent(int32 SlotIndex)
{
	FGameplayEventData Payload;
	Payload.EventMagnitude = static_cast<float>(SlotIndex);
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetPawnChecked<APawn>(), SFGameplayTags::GameplayEvent_UseQuickbar, Payload);
}

void USFHeroComponent::InitializeHUD()
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	ASFPlayerController* PC = GetController<ASFPlayerController>();
	if (!PC)
	{
		return;
	}

	ASFHUD* HUD = PC->GetHUD<ASFHUD>();
	if (!HUD)
	{
		return;
	}

	ASFPlayerState* PS = PC->GetPlayerState<ASFPlayerState>();
	if (!PS)
	{
		return;
	}

	USFAbilitySystemComponent* ASC = PS->GetSFAbilitySystemComponent();
	USFPrimarySet_Hero* PrimarySet = const_cast<USFPrimarySet_Hero*>(PS->GetPrimarySet());
	USFCombatSet_Hero* CombatSet = const_cast<USFCombatSet_Hero*>(PS->GetCombatSet());
    
	if (ASC && PrimarySet && CombatSet)
	{
		HUD->InitOverlay(PC, PS, ASC, PrimarySet, CombatSet, GetWorld()->GetGameState());
        
		UE_LOG(LogSF, Log, TEXT("HUD Overlay initialized for %s"), *PS->GetPlayerName());
	}
}

TSubclassOf<USFCameraMode> USFHeroComponent::DetermineCameraMode()
{
	if (AbilityCameraMode)
	{
		return AbilityCameraMode;
	}

	
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return nullptr;
	}

	if (const USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const USFPawnData* PawnData = PawnExtComp->GetPawnData<USFPawnData>())
		{
			if (PawnData->DefaultCameraMode)
			{
				return PawnData->DefaultCameraMode;
			}
		}
	}

	return nullptr;
}

void USFHeroComponent::SetMouseSensitivity(float NewSensitivity)
{
	MouseSensitivity = FMath::Max(NewSensitivity, 0.01f);
}

void USFHeroComponent::SetAbilityCameraMode(TSubclassOf<USFCameraMode> CameraMode, const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (CameraMode)
	{
		AbilityCameraMode = CameraMode;
		AbilityCameraModeOwningSpecHandle = OwningSpecHandle;
	}
}

void USFHeroComponent::ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (AbilityCameraModeOwningSpecHandle == OwningSpecHandle)
	{
		AbilityCameraMode = nullptr;
		AbilityCameraModeOwningSpecHandle = FGameplayAbilitySpecHandle();
	}
}

void USFHeroComponent::DisableAbilityCameraYawLimits()
{
	if (APawn* Pawn = GetPawn<APawn>())
	{
		if (USFCameraComponent* CameraComp = USFCameraComponent::FindCameraComponent(Pawn))
		{
			CameraComp->DisableAllYawLimitsTemporarily();
		}
	}
}

void USFHeroComponent::DisableAbilityCameraYawLimitsForMode(TSubclassOf<USFCameraMode> CameraModeClass)
{
	if (!CameraModeClass)
	{
		return;
	}

	if (APawn* Pawn = GetPawn<APawn>())
	{
		if (USFCameraComponent* CameraComp = USFCameraComponent::FindCameraComponent(Pawn))
		{
			CameraComp->DisableYawLimitsForMode(CameraModeClass);
		}
	}
}

