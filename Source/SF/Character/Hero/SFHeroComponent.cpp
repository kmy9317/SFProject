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
#include "Camera/SFCameraMode.h"

const FName USFHeroComponent::NAME_ActorFeatureName("Hero");

USFHeroComponent::USFHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//AbilityCameraMode = nullptr;
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
}

void USFHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == USFPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == SFGameplayTags::InitState_DataInitialized)
		{
			// LCPawnExtensionComponent의 DataInitialized 상태 변화 관찰 후, LCHeroComponent도 DataInitialized 상태로 변경
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
				
				USFInputComponent* LCIC = Cast<USFInputComponent>(PlayerInputComponent);
				if (ensureMsgf(LCIC, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to USFInputComponent or a subclass of it.")))
				{
					TArray<uint32> BindHandles;
					LCIC->BindAbilityActions(InputConfig, this,&ThisClass::Input_AbilityInputTagStarted, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

					LCIC->BindNativeAction(InputConfig, SFGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move, /*bLogIfNotFound=*/ false);
					LCIC->BindNativeAction(InputConfig, SFGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse, /*bLogIfNotFound=*/ false);
					LCIC->BindNativeAction(InputConfig, SFGameplayTags::InputTag_Crouch, ETriggerEvent::Triggered, this, &ThisClass::Input_Crouch, /*bLogIfNotFound=*/ false);
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
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	
	if (Controller)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			Pawn->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			Pawn->AddMovementInput(MovementDirection, Value.Y);
		}
	}
}

void USFHeroComponent::Input_LookMouse(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}
	
	const FVector2D Value = InputActionValue.Get<FVector2D>();

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X);
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y);
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

