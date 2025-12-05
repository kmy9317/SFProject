#include "SFEnemyWidgetComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Enemy/SFPrimarySet_Enemy.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "UI/Common/CommonBarBase.h"
#include "UI/Enemy/USFEnemyWidget.h"

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
    InitializeWidget();
}

void USFEnemyWidgetComponent::OnHealthChanged(const FOnAttributeChangeData& OnAttributeChangeData)
{
    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
    if (IsValid(ASC))
    {
        const USFPrimarySet_Enemy* PrimarySet = ASC->GetSet<USFPrimarySet_Enemy>();
        if (IsValid(PrimarySet))
        {
           int Health = PrimarySet->GetHealth();
           int MaxHealth = PrimarySet->GetMaxHealth();
           float Percent = Health / MaxHealth;
           EnemyWidget->SetPercentVisuals(Percent);
            
        }
    }
    SetVisibility(true);
    CurrentVisibleTime  =0.f;
    
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
            ASC->GetGameplayAttributeValueChangeDelegate(ASC->GetSet<USFPrimarySet_Enemy>()->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
        }
    }
    EnemyWidget->SetPercentVisuals(1);
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