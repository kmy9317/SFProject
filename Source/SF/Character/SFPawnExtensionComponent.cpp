#include "SFPawnExtensionComponent.h"

#include "AbilitySystem/SFAbilitySet.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "SF/System/SFInitGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Character/SFPawnData.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"

/** feature name은 component 단위니깐 component를 빼고 pawn extension만 넣은 것을 확인할 수 있다 */
const FName USFPawnExtensionComponent::NAME_ActorFeatureName("PawnExtension");

USFPawnExtensionComponent::USFPawnExtensionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);

	PawnData = nullptr;
	AbilitySystemComponent = nullptr;
}

void USFPawnExtensionComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USFPawnExtensionComponent, PawnData);
}

void USFPawnExtensionComponent::OnRegister()
{
	// OnRegister단계에서 컴포넌트를 부착
	// 컴포넌트가 생성되는 극 초반 단계에서 호출
	Super::OnRegister();

	const APawn* Pawn = GetPawn<APawn>();
	ensureAlwaysMsgf((Pawn != nullptr), TEXT("SFPawnExtensionComponent on [%s] can only be added to Pawn actors."), *GetNameSafe(GetOwner()));

	TArray<UActorComponent*> PawnExtensionComponents;
	Pawn->GetComponents(USFPawnExtensionComponent::StaticClass(), PawnExtensionComponents);
	ensureAlwaysMsgf((PawnExtensionComponents.Num() == 1), TEXT("Only one SFPawnExtensionComponent should exist on [%s]."), *GetNameSafe(GetOwner()));

	// GameFrameworkComponentManager에 InitState 사용을 위해 등록 진행
	// - 등록은 상속받았던 IGameFrameworkInitStateInterface 메서드 RegisterInitStateFeature()를 활용
	RegisterInitStateFeature();
}

void USFPawnExtensionComponent::BeginPlay()
{
	Super::BeginPlay();

	// FeatureName에 NAME_None을 넣으면, Actor에 등록된 모든  Feature Component의 InitState 상태를 관찰하겠다는 의미
	// 내부적으로 한 엑터를 구분해주는 FObjectKey의 데이터로 FActorFeatureData내 델리게이트를 Array로 넣어준다
	BindOnActorInitStateChanged(NAME_None, FGameplayTag(), false);
	
	// InitState_Spawned로 상태 변환:
	// - TryToChangeInitState는 아래와 같이 진행된다:
	//   1. CanChangeInitState로 상태 변환 가능성 유무 판단
	//   2. HandleChangeInitState로 내부 상태 변경 (Feature Component)
	//   3. BindOnActorInitStateChanged로 Bind된 Delegate를 조건에 맞게 호출
	//      - SFPawnExtensionComponent의 경우,  Actor의 모든 Feature 상태 변화에 대해 OnActorInitStateChanged()가 호출됨
	ensure(TryToChangeInitState(SFGameplayTags::InitState_Spawned));

	// - 이 함수를 ForceUpdateInitState와 같은 느낌으로 이해할 수 있다
	// - 현재 강제 업데이트 진행 (물론 CanChangeInitState와 HandleChangleInitState를 진행해준다)
	CheckDefaultInitialization();
}

void USFPawnExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UninitializeAbilitySystem();
	// 해당 컴포넌트를 등록한 프레임워크 매니저 컴포넌트에서 이 컴포넌트 등록을 해제하는 부분
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void USFPawnExtensionComponent::SetPawnData(const USFPawnData* InPawnData)
{
	check(InPawnData);

	APawn* Pawn = GetPawnChecked<APawn>();

	// Pawn에 대해 Authority가 없을 경우, SetPawnData는 진행하지 않음
	if (Pawn->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (PawnData)
	{
		UE_LOG(LogTemp, Error, TEXT("Trying to set PawnData [%s] on pawn [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(Pawn), *GetNameSafe(PawnData));
		return;
	}

	PawnData = InPawnData;

	Pawn->ForceNetUpdate();

	CheckDefaultInitialization();
}

void USFPawnExtensionComponent::OnRep_PawnData()
{
	CheckDefaultInitialization();
}

void USFPawnExtensionComponent::InitializeAbilitySystem(USFAbilitySystemComponent* InASC, AActor* InOwnerActor)
{
	check(InASC);
	check(InOwnerActor);

	if (AbilitySystemComponent == InASC)
	{
		return;
	}

	if (AbilitySystemComponent)
	{
		UninitializeAbilitySystem();
	}

	APawn* Pawn = GetPawnChecked<APawn>();
	AActor* ExistingAvatar = InASC->GetAvatarActor();

	UE_LOG(LogTemp, Verbose, TEXT("Setting up ASC [%s] on pawn [%s] owner [%s], existing [%s] "), *GetNameSafe(InASC), *GetNameSafe(Pawn), *GetNameSafe(InOwnerActor), *GetNameSafe(ExistingAvatar));

	if ((ExistingAvatar != nullptr) && (ExistingAvatar != Pawn))
	{
		UE_LOG(LogTemp, Log, TEXT("Existing avatar (authority=%d)"), ExistingAvatar->HasAuthority() ? 1 : 0);

		// There is already a pawn acting as the ASC's avatar, so we need to kick it out
		// This can happen on clients if they're lagged: their new pawn is spawned + possessed before the dead one is removed
		ensure(!ExistingAvatar->HasAuthority());

		if (USFPawnExtensionComponent* OtherExtensionComponent = FindPawnExtensionComponent(ExistingAvatar))
		{
			OtherExtensionComponent->UninitializeAbilitySystem();
		}
	}

	// ASC를 업데이트하고 InitAbilityActorInfo를 Pawn과 같이 호출하여 AvatarActor를 Pawn으로 업데이트 해준다
	AbilitySystemComponent = InASC;
	AbilitySystemComponent->InitAbilityActorInfo(InOwnerActor, Pawn);
	
	// 해당 시점에 캐릭터에 대해 RegisterGameplayTagEvent 등록을 해둘 수 있음
	OnAbilitySystemInitialized.Broadcast();
}

void USFPawnExtensionComponent::UninitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	if (AbilitySystemComponent->GetAvatarActor() == GetOwner())
	{
		FGameplayTagContainer AbilityTypesToIgnore;
		//AbilityTypesToIgnore.AddTag(SFGameplayTags::Ability_Behavior_SurvivesDeath);

		AbilitySystemComponent->CancelAbilities(nullptr, &AbilityTypesToIgnore);
		AbilitySystemComponent->ClearAbilityInput();
		AbilitySystemComponent->RemoveAllGameplayCues();

		if (AbilitySystemComponent->GetOwnerActor() != nullptr)
		{
			AbilitySystemComponent->SetAvatarActor(nullptr);
		}
		else
		{
			// If the ASC doesn't have a valid owner, we need to clear *all* actor info, not just the avatar pairing
			AbilitySystemComponent->ClearActorInfo();
		}

		OnAbilitySystemUninitialized.Broadcast();
	}

	AbilitySystemComponent = nullptr;
}

void USFPawnExtensionComponent::HandleControllerChanged()
{
	if (AbilitySystemComponent && (AbilitySystemComponent->GetAvatarActor() == GetPawnChecked<APawn>()))
	{
		ensure(AbilitySystemComponent->AbilityActorInfo->OwnerActor == AbilitySystemComponent->GetOwnerActor());
		if (AbilitySystemComponent->GetOwnerActor() == nullptr)
		{
			UninitializeAbilitySystem();
		}
		else
		{
			AbilitySystemComponent->RefreshAbilityActorInfo();
		}
	}

	CheckDefaultInitialization();
}

void USFPawnExtensionComponent::HandlePlayerStateReplicated()
{
	CheckDefaultInitialization();
}

void USFPawnExtensionComponent::SetupPlayerInputComponent()
{
	CheckDefaultInitialization();
}

void USFPawnExtensionComponent::CheckDefaultInitialization()
{
	// PawnExtensionComponent는 Feature Component들의 초기화를 관장하는 Component
	// - 따라서, Actor에 바인딩된 Feature Component들에 대해 CheckDefaultInitialization을 호출해주도록 한다 (ForceUpdate 느낌)
	CheckDefaultInitializationForImplementers();

	// 사용자 정의 InitState를 직접 넘겨줘야 한다
	static const TArray<FGameplayTag> StateChain = { SFGameplayTags::InitState_Spawned, SFGameplayTags::InitState_DataAvailable, SFGameplayTags::InitState_DataInitialized, SFGameplayTags::InitState_GameplayReady };

	// CanChangeInitState()와 HandleChangeInitState() 그리고 ChangeFeatureInitState 호출을 통한 OnActorInitStateChanged Delegate 호출까지 진행
	//   - 계속 가능한 상태까지 한번 업데이트 한다
	//   - 업데이트가 멈추면 어디선가 다시 호출해야 함(chaining)
	// 이 State가 비어있는 상태의 태그면 state는 변화하지 않음
	ContinueInitStateChain(StateChain);
}

bool USFPawnExtensionComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	// InitState_Spawned 초기화
	if (!CurrentState.IsValid() && DesiredState == SFGameplayTags::InitState_Spawned)
	{
		// Pawn이 잘 세팅만 되어있으면 바로 Spawned로 넘어감
		if (Pawn)
		{
			return true;
		}
	}
	
	// Spawned -> DataAvailable
	if (CurrentState == SFGameplayTags::InitState_Spawned && DesiredState == SFGameplayTags::InitState_DataAvailable)
	{
		// PawnData 설정되지 않았으면 대기
		if (!PawnData)
		{
			return false;
		}

		const bool bHasAuthority = Pawn->HasAuthority();
		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();

		if (bHasAuthority || bIsLocallyControlled)
		{
			// Check for being possessed by a controller.
			if (!GetController<AController>())
			{
				return false;
			}
		}

		return true;
	}
	
	// DataAvailable -> DataInitialized
	else if (CurrentState == SFGameplayTags::InitState_DataAvailable && DesiredState == SFGameplayTags::InitState_DataInitialized)
	{
		// Actor에 바인드된 모든 Feature들이 DataAvailable 상태일 때, DataInitialized로 넘어감
		return Manager->HaveAllFeaturesReachedInitState(Pawn, SFGameplayTags::InitState_DataAvailable);
	}
	
	// DataInitialized -> GameplayReady
	else if (CurrentState == SFGameplayTags::InitState_DataInitialized && DesiredState == SFGameplayTags::InitState_GameplayReady)
	{
		return true;
	}
	
	// 위의 선형적인(linear) transition이 아니면 false
	return false;
}

void USFPawnExtensionComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (DesiredState == SFGameplayTags::InitState_DataInitialized)
	{
		// This is currently all handled by other components listening to this state change
	}
}

void USFPawnExtensionComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName != NAME_ActorFeatureName)
	{
		// SFPawnExtensionComponent는 다른 Feature Component들의 상태가 DataAvailable인지 관찰하여, Sync를 맞추는 구간이 있다 (CanChangeInitState)
		// - 이를 가능케하기 위해, OnActorInitStateChanged에서는 DataAvailable에 대해 지속적으로 CheckDefaultInitialization을 호출하여, 상태를 확인한다
		if (Params.FeatureState == SFGameplayTags::InitState_DataAvailable)
		{
			CheckDefaultInitialization();
		}
	}
}

void USFPawnExtensionComponent::OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate)
{
	// OnAbilitySystemInitialized에 UObject가 바인딩되어 있지 않으면 추가 (Uniqueness)
	if (!OnAbilitySystemInitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemInitialized.Add(Delegate);
	}

	// 이미 ASC가 설정되었으면, Delegate에 추가하는게 아닌 바로 호출
	if (AbilitySystemComponent)
	{
		Delegate.Execute();
	}
}

void USFPawnExtensionComponent::OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnAbilitySystemUninitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemUninitialized.Add(Delegate);
	}
}

