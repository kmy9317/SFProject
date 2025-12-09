#include "SFEnemyWidgetComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystem/Attributes/Enemy/SFPrimarySet_Enemy.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "UI/Common/CommonBarBase.h"


USFEnemyWidgetComponent::USFEnemyWidgetComponent()
{
    SetWidgetSpace(EWidgetSpace::World);
    PrimaryComponentTick.bCanEverTick = true;

    SetDrawSize(FVector2D(100.f, 10.f));
    SetPivot(FVector2D(0.5f, 0.f));
    SetTwoSided(true);
    SetVisibility(false);

    VisibleDurationAfterHit = 3.f;
    CurrentVisibleTime = 0.f;
    MaxRenderDistance = 3000.f;
    HideOnAngle = 90.f;
}

void USFEnemyWidgetComponent::BeginPlay()
{
    Super::BeginPlay();

    TryInitializeWidget();
}

void USFEnemyWidgetComponent::TryInitializeWidget()
{
    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
    if (IsValid(ASC))
    {
        InitializeWidget();
    }
    else
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::TryInitializeWidget);
    }
}
void USFEnemyWidgetComponent::OnHealthChanged(const FOnAttributeChangeData& OnAttributeChangeData)
{
    UpdateHealthPercent();
    
    if (bEngagedByLocalPlayer)
    {
        ShowHealthBar();
        ResetEngagementTimer();
    }
}

void USFEnemyWidgetComponent::InitWidget()
{
    Super::InitWidget();

    EnemyWidget = Cast<UCommonBarBase>(GetUserWidgetObject());
}

void USFEnemyWidgetComponent::InitializeWidget()
{
    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
    if (IsValid(ASC))
    {
        const USFPrimarySet_Enemy* PrimarySet = ASC->GetSet<USFPrimarySet_Enemy>();
        if (IsValid(PrimarySet))
        {
           
            ASC->GetGameplayAttributeValueChangeDelegate(PrimarySet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);

            // MaxHealth 캐싱
            CachedMaxHealth = PrimarySet->GetMaxHealth();
        }
    }

    // 이 컴포넌트의 주인(몬스터)이 캐릭터인지 확인
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter && OwnerCharacter->GetCapsuleComponent())
    {
        // 캡슐의 절반 높이 (중심에서 머리끝까지)를 가져옴
        float CapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

        // 위젯의 새로운 위치 설정
        // X, Y는 0 (중앙), Z는 캡슐 높이 + 우리가 설정한 여유 공간(Offset)
        SetRelativeLocation(FVector(0.f, 0.f, CapsuleHalfHeight + WidgetVerticalOffset));
    }
    
    if (EnemyWidget)
    {
        EnemyWidget->SetBarColor(HealthBarColor);
        EnemyWidget->SetPercentVisuals(1.f);
    }
}

void USFEnemyWidgetComponent::MarkAsAttackedByLocalPlayer()
{
    bEngagedByLocalPlayer = true;
    ShowHealthBar();
    ResetEngagementTimer();
}

void USFEnemyWidgetComponent::ShowHealthBar()
{
    SetVisibility(true);
    CurrentVisibleTime = VisibleDurationAfterHit;
}

void USFEnemyWidgetComponent::OnEngagementExpired()
{
    bEngagedByLocalPlayer = false;
    SetVisibility(false);
}

//이게 교전 중일때 일정 시간 데미지 안받으면 RESET
void USFEnemyWidgetComponent::ResetEngagementTimer()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(EngagementTimerHandle);
        World->GetTimerManager().SetTimer(
            EngagementTimerHandle,
            this,
            &ThisClass::OnEngagementExpired,
            VisibleDurationAfterHit,
            false
        );
    }
}

void USFEnemyWidgetComponent::UpdateHealthPercent()
{
    if (!EnemyWidget)
    {
        return;
    }

    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
    if (IsValid(ASC))
    {
        const USFPrimarySet_Enemy* PrimarySet = ASC->GetSet<USFPrimarySet_Enemy>();
        if (IsValid(PrimarySet))
        {
            float Health = PrimarySet->GetHealth();
            float MaxHealth = PrimarySet->GetMaxHealth();

            if (MaxHealth > 0.f)
            {
                float Percent = Health / MaxHealth;
                EnemyWidget->SetPercentVisuals(Percent);
            }
        }
    }
}

void USFEnemyWidgetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsVisible())
    {
        return;
    }
       
    FaceToCamera();
    UpdateVisibility(DeltaTime);
}

void USFEnemyWidgetComponent::FaceToCamera()
{
    APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
    if (!Cam)
    {
        return;
    }

    FVector Dir = Cam->GetCameraLocation() - GetComponentLocation();
    FRotator Rot = FRotationMatrix::MakeFromX(Dir).Rotator();
    Rot.Pitch = 0.f; 
    SetWorldRotation(Rot);
}

void USFEnemyWidgetComponent::UpdateVisibility(float DeltaTime)
{
    APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
    if (!Cam)
    {
        return;
    }

    FVector CamLoc = Cam->GetCameraLocation();
    FVector Loc = GetComponentLocation();
    float Dist = FVector::Dist(CamLoc, Loc);

    if (CurrentVisibleTime > 0.f)
    {
        CurrentVisibleTime -= DeltaTime;
        return; 
    }

    if (Dist > MaxRenderDistance)
    {
        SetVisibility(false);
        return;
    }

    FVector CamForward = Cam->GetActorForwardVector();
    FVector ToEnemy = (Loc - CamLoc).GetSafeNormal();
    float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(CamForward, ToEnemy)));

    if (Angle > HideOnAngle)
    {
        SetVisibility(false);
    }
}