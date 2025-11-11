#include "SFHeroDisplay.h"

#include "SFLogChannels.h"
#include "Camera/CameraComponent.h"
#include "Character/Hero/SFHeroDefinition.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "UI/Lobby/SFPlayerInfoWidget.h"


ASFHeroDisplay::ASFHeroDisplay()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root Comp"));

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("Mesh Component");
	MeshComponent->SetupAttachment(GetRootComponent());
	
	ViewCameraComponent = CreateDefaultSubobject<UCameraComponent>("View Camera Component");
	ViewCameraComponent->SetupAttachment(GetRootComponent());

	PlayerInfoWidget = CreateDefaultSubobject<UWidgetComponent>("PlayerInfo Widget");
	PlayerInfoWidget->SetupAttachment(MeshComponent); // 캐릭터 머리 위에 표시
	PlayerInfoWidget->SetWidgetSpace(EWidgetSpace::Screen); // ScreenSpace
	PlayerInfoWidget->SetDrawAtDesiredSize(true); // Draw at desired size 체크
	PlayerInfoWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f)); // 머리 위
	PlayerInfoWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASFHeroDisplay::BeginPlay()
{
	Super::BeginPlay();

	EnsureWidgetInitialized();
}

void ASFHeroDisplay::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASFHeroDisplay, PlayerInfo);
	DOREPLIFETIME(ASFHeroDisplay, CurrentHeroDefinition);
}

void ASFHeroDisplay::UpdateHeroDefination(const USFHeroDefinition* HeroDefinition)
{
	// TODO : CurrentHeroDefintion 이 nullptr일 경우 추가적인 처리 고려
	if (!HasAuthority() || !HeroDefinition)
	{
		return;
	}

	if (CurrentHeroDefinition != HeroDefinition)
	{
		CurrentHeroDefinition = HeroDefinition;
		ApplyHeroConfiguration();
	}
}

void ASFHeroDisplay::UpdatePlayerInfo(const FSFPlayerInfo& NewPlayerInfo)
{
	if (!HasAuthority())
	{
		return;
	}

	// 변경사항 있을 때만 업데이트
	if (PlayerInfo != NewPlayerInfo)
	{
		PlayerInfo = NewPlayerInfo;

		// 서버에서도 위젯 업데이트(listen server)
		UpdatePlayerInfoWidget();
	}
}

void ASFHeroDisplay::OnRep_PlayerInfo()
{
	// 클라이언트에서 위젯 업데이트
	UpdatePlayerInfoWidget();
}

void ASFHeroDisplay::OnRep_CurrentHeroDefinition()
{
	ApplyHeroConfiguration();
}

void ASFHeroDisplay::UpdatePlayerInfoWidget()
{
	EnsureWidgetInitialized();
	
	// 위젯이 USFPlayerInfoWidget인지 확인
	if (USFPlayerInfoWidget* InfoWidget = Cast<USFPlayerInfoWidget>(PlayerInfoWidget->GetWidget()))
	{
		InfoWidget->UpdatePlayerInfo(PlayerInfo);
	}

	PlayerInfoWidget->SetVisibility(true);
}

void ASFHeroDisplay::ApplyHeroConfiguration()
{
	// TODO : CurrentHeroDefintion 이 nullptr일 경우 추가적인 처리 고려
	if (!CurrentHeroDefinition)
	{
		return;
	}

	MeshComponent->SetSkeletalMesh(CurrentHeroDefinition->LoadDisplayMesh());
	MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	MeshComponent->SetAnimInstanceClass(CurrentHeroDefinition->LoadDisplayAnimationBP());
}

void ASFHeroDisplay::EnsureWidgetInitialized()
{
	if (!PlayerInfoWidget)
	{
		return;
	}
	if (!PlayerInfoWidget->GetWidget())
	{
		if (!PlayerInfoWidget->GetWidgetClass())
		{
			UE_LOG(LogSF, Error, TEXT("[HeroDisplay] WidgetClass not set on %s"), *GetName());
			return;
		}
		PlayerInfoWidget->InitWidget();
	}
}

