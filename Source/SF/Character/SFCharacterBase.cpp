#include "Character/SFCharacterBase.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "SFPawnExtensionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "Hero/SFHeroComponent.h"
#include "Player/SFPlayerState.h"
#include "Team/SFTeamTypes.h"

ASFCharacterBase::ASFCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	// SFPawnExtensionComponent만 native c++로 설정해 줌. 해당 컴포넌트가 다른 컴포넌트들의
	// 초기화를 담당하는 컴포넌트이기 떄문에 가장 먼저 생성해 주어야 한다. 엔진에서는  native c++의 컴포넌트가 먼저 생성되고 
	// 이후에 블루프린트에서 만든 컴포넌트들이 생성되도록 구현되어 있다. 
	PawnExtComponent = CreateDefaultSubobject<USFPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	PawnExtComponent->OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	PawnExtComponent->OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	
	// Motion Warping Component 초기화 (소울류 공격 시 적에게 달라붙는 기능)
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
}

void ASFCharacterBase::OnAbilitySystemInitialized()
{
	// TODO : 파생 클래스에서 ASC 의존적 컴포넌트들 초기화 로직 등 수행 가능 
}

void ASFCharacterBase::OnAbilitySystemUninitialized()
{
	// TODO : 파생 클래스에서 ASC 의존적 컴포넌트들 초기화 해제
}

USFAbilitySystemComponent* ASFCharacterBase::GetSFAbilitySystemComponent() const
{
	return Cast<USFAbilitySystemComponent>(GetAbilitySystemComponent());
}

UAbilitySystemComponent* ASFCharacterBase::GetAbilitySystemComponent() const
{
	if (PawnExtComponent == nullptr)
	{
		return nullptr;
	}

	return PawnExtComponent->GetSFAbilitySystemComponent();
}

void ASFCharacterBase::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (const USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent())
	{
		SFASC->GetOwnedGameplayTags(TagContainer);
	}
}

bool ASFCharacterBase::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	if (const USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent())
	{
		return SFASC->HasMatchingGameplayTag(TagToCheck);
	}

	return false;
}

bool ASFCharacterBase::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent())
	{
		return SFASC->HasAllMatchingGameplayTags(TagContainer);
	}

	return false;
}

bool ASFCharacterBase::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent())
	{
		return SFASC->HasAnyMatchingGameplayTags(TagContainer);
	}

	return false;
}

void ASFCharacterBase::ToggleCrouch()
{
	const UCharacterMovementComponent* MoveComp = CastChecked<UCharacterMovementComponent>(GetCharacterMovement());

	if (IsCrouched() || MoveComp->bWantsToCrouch)
	{
		UnCrouch();
	}
	else if (MoveComp->IsMovingOnGround())
	{
		Crouch();
	}
}

void ASFCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void ASFCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	PawnExtComponent->HandleControllerChanged();
}

void ASFCharacterBase::UnPossessed()
{
	Super::UnPossessed();

	PawnExtComponent->HandleControllerChanged();
}

void ASFCharacterBase::OnRep_Controller()
{
	Super::OnRep_Controller();

	PawnExtComponent->HandleControllerChanged();
}

void ASFCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (PawnExtComponent)
	{
		PawnExtComponent->HandlePlayerStateReplicated();
	}

	// Binds the ASC to the Pawn, linking the PlayerState's ASC to the character instance.
	// This is the most reliable place to do this on clients.
	if (ASFPlayerState* PS = GetPlayerState<ASFPlayerState>())
	{
		if (PawnExtComponent)
		{
			PawnExtComponent->InitializeAbilitySystem(PS->GetSFAbilitySystemComponent(), PS);
		}
	}
}

void ASFCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PawnExtComponent->SetupPlayerInputComponent();
}

void ASFCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateAnimValue();
}

bool ASFCharacterBase::IsFalling() const
{
	if (const UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		return CMC->IsFalling();
	}

	return false;
}

FVector ASFCharacterBase::GetLastInputDirection() const
{
	if (const USFHeroComponent* HeroComp = USFHeroComponent::FindHeroComponent(this))
	{
		return HeroComp->GetLastInputDirection();
	}

	// AI나 HeroComponent가 없는 경우 Zero 반환
	return FVector::ZeroVector;
}

void ASFCharacterBase::UpdateAnimValue()
{
	const FVector Velocity = GetVelocity();
	const FRotator Rotation = GetActorRotation();

	GroundSpeed = FVector(Velocity.X, Velocity.Y, 0.0).Size();

	const FVector RelativeVelocity = Rotation.UnrotateVector(Velocity);
	Direction = FMath::Atan2(RelativeVelocity.Y, RelativeVelocity.X) * (180.0f / PI);
}

FGenericTeamId ASFCharacterBase::GetGenericTeamId() const
{
	// todo 이거 서브 클래스에서 getter를 무조건 다시 해야하는데
	//Player는 PlayerState의 TeamID를 return 해야하고 Enemy는 Controller의 값을 return 해야함.
	return FGenericTeamId(SFTeamID::NoTeam);
}

