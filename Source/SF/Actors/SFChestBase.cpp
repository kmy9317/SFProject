#include "SFChestBase.h"

#include "Components/ArrowComponent.h"
#include "Net/UnrealNetwork.h"

ASFChestBase::ASFChestBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	SetRootComponent(ArrowComponent);
	
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(GetRootComponent());
	MeshComponent->SetCollisionProfileName(TEXT("Interactable"));
	MeshComponent->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	MeshComponent->SetCanEverAffectNavigation(true);
	MeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesAndRefreshBonesWhenPlayingMontages;

	bShouldConsume = true;
}

void ASFChestBase::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	// TODO : 추후 강화 등급에 따라 상자를 열고 난 이후 플레이어의 강화 선택지의 목록을 적절하게 보이도록 미리 설정
}

void ASFChestBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ChestState);
}

FSFInteractionInfo ASFChestBase::GetPreInteractionInfo(const FSFInteractionQuery& InteractionQuery) const
{
	switch (ChestState)
	{
	case ESFChestState::Open:		return OpenedInteractionInfo;
	case ESFChestState::Close:	return ClosedInteractionInfo;
	default:					return FSFInteractionInfo();
	}
}

void ASFChestBase::GetMeshComponents(TArray<UMeshComponent*>& OutMeshComponents) const
{
	Super::GetMeshComponents(OutMeshComponents);
}

void ASFChestBase::SetChestState(ESFChestState NewChestState)
{
	if (!HasAuthority() || NewChestState == ChestState)
	{
		return;
	}
	
	ChestState = NewChestState;
	OnRep_ChestState();
}

void ASFChestBase::OnRep_ChestState()
{
	if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
	{
		UAnimMontage* SelectedMontage = nullptr;
		switch (ChestState)
		{
		case ESFChestState::Open:
			SelectedMontage = OpenMontage;
			break;
		case ESFChestState::Close:
			SelectedMontage = CloseMontage;
			break;
		}

		if (SelectedMontage)
		{
			AnimInstance->Montage_Play(SelectedMontage);
		}
	}
}

