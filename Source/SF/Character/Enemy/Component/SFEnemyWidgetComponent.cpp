#include "SFEnemyWidgetComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Enemy/SFPrimarySet_Enemy.h"
#include "Character/SFPawnExtensionComponent.h" // [필수] 헤더 추가
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/SFCharacterBase.h"
#include "Character/Enemy/SFEnemyData.h"
#include "Character/Enemy/SFEnemyGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "UI/Common/CommonBarBase.h"

USFEnemyWidgetComponent::USFEnemyWidgetComponent()
{
    SetWidgetSpace(EWidgetSpace::World);
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false; 

    SetDrawSize(FVector2D(100.f, 10.f));
    SetPivot(FVector2D(0.5f, 0.f));
    SetTwoSided(true);
    SetVisibility(false);
}

void USFEnemyWidgetComponent::InitWidget()
{
    Super::InitWidget();
    EnemyWidget = Cast<UCommonBarBase>(GetUserWidgetObject());
}

void USFEnemyWidgetComponent::InitializeWidget()
{
   
    ASFCharacterBase* OwnerCharacter = Cast<ASFCharacterBase>(GetOwner());
    if (!OwnerCharacter) return;
    if(USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(OwnerCharacter))
    {
        const USFEnemyData* EnemyData = PawnExtComp->GetPawnData<USFEnemyData>();
        if (EnemyData && EnemyData->EnemyType == SFGameplayTags::Enemy_Type_Boss)
        {
            return;
        }
    }
    UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
    if (IsValid(ASC))
    {
        const USFPrimarySet_Enemy* PrimarySet = ASC->GetSet<USFPrimarySet_Enemy>();
        if (IsValid(PrimarySet))
        {
            ASC->GetGameplayAttributeValueChangeDelegate(PrimarySet->GetHealthAttribute())
               .AddUObject(this, &ThisClass::OnHealthChanged);
        }
    }
    if (OwnerCharacter && OwnerCharacter->GetCapsuleComponent())
    {
        float CapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        SetRelativeLocation(FVector(0.f, 0.f, CapsuleHalfHeight + WidgetVerticalOffset));
    }
    
    if (EnemyWidget)
    {
        EnemyWidget->SetBarColor(HealthBarColor);
        EnemyWidget->SetPercentVisuals(1.f);
        UpdateHealthPercent(); 
    }
}

void USFEnemyWidgetComponent::BeginPlay()
{
    Super::BeginPlay();
    
}

void USFEnemyWidgetComponent::OnHealthChanged(const FOnAttributeChangeData& OnAttributeChangeData)
{
    UpdateHealthPercent();
    
    if (OnAttributeChangeData.NewValue <= 0.0f)
    {
        OnEngagementExpired(); 
        return;
    }
    
    if (OnAttributeChangeData.OldValue > OnAttributeChangeData.NewValue)
    {
        ShowHealthBar();
        ResetEngagementTimer();
    }
}

void USFEnemyWidgetComponent::ShowHealthBar()
{
    SetVisibility(true);
    CurrentVisibleTime = VisibleDurationAfterHit;
    SetComponentTickEnabled(true);
}

void USFEnemyWidgetComponent::OnEngagementExpired()
{
    SetVisibility(false);
    SetComponentTickEnabled(false);
}

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
    if (!EnemyWidget) return;

    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
    if (IsValid(ASC))
    {
        const USFPrimarySet_Enemy* PrimarySet = ASC->GetSet<USFPrimarySet_Enemy>();
        if (IsValid(PrimarySet))
        {
            float Percent = PrimarySet->GetHealth() / PrimarySet->GetMaxHealth();
            EnemyWidget->SetPercentVisuals(Percent);
        }
    }
}

void USFEnemyWidgetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    FaceToCamera();
    UpdateVisibility(DeltaTime);
}

void USFEnemyWidgetComponent::FaceToCamera()
{
    APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
    if (!Cam) return;

    FVector Dir = Cam->GetCameraLocation() - GetComponentLocation();
    FRotator Rot = FRotationMatrix::MakeFromX(Dir).Rotator();
    Rot.Pitch = 0.f; 
    SetWorldRotation(Rot);
}

void USFEnemyWidgetComponent::UpdateVisibility(float DeltaTime)
{
    if (CurrentVisibleTime > 0.f)
    {
        CurrentVisibleTime -= DeltaTime;
    }
    else
    {
        OnEngagementExpired(); 
        return;
    }

    // 거리 및 각도 체크 
    APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
    if (!Cam) return;

    float Dist = FVector::Dist(Cam->GetCameraLocation(), GetComponentLocation());
    if (Dist > MaxRenderDistance)
    {
        SetVisibility(false); 
        return;
    }
}