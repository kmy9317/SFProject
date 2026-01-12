#include "SFMinimapWidget.h"
#include "SFMiniMapIcon.h"
#include "Components/CanvasPanel.h"
#include "System/SFMinimapSubsystem.h"

void USFMinimapWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UWorld* World = GetWorld();
    if (World)
    {
        MiniMapSubsystem = World->GetSubsystem<USFMinimapSubsystem>();
        if (MiniMapSubsystem)
        {
            MiniMapSubsystem->OnTargetRegistered.AddDynamic(this, &USFMinimapWidget::OnTargetRegistered);
            MiniMapSubsystem->OnTargetUnregistered.AddDynamic(this, &USFMinimapWidget::OnTargetUnregistered);
        }
    }
}

void USFMinimapWidget::NativeDestruct()
{
    if (MiniMapSubsystem)
    {
        MiniMapSubsystem->OnTargetRegistered.RemoveAll(this);
        MiniMapSubsystem->OnTargetUnregistered.RemoveAll(this);
    }
    Super::NativeDestruct();
}

void USFMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdateIconPositions();
}

void USFMinimapWidget::OnTargetRegistered(TScriptInterface<ISFMiniMapTrackable> Target)
{
    if (!Target.GetObject() || !IconWidgetClass || !MiniMapCanvas) return;
    
    if (TargetIcons.Contains(Target.GetObject())) return;
    
    USFMiniMapIcon* NewIcon = CreateWidget<USFMiniMapIcon>(this, IconWidgetClass);
    if (NewIcon)
    {
        NewIcon->SetTarget(Target);
        MiniMapCanvas->AddChild(NewIcon);
        TargetIcons.Add(Target.GetObject(), NewIcon);
    }
}

void USFMinimapWidget::OnTargetUnregistered(TScriptInterface<ISFMiniMapTrackable> Target)
{
    if (!Target.GetObject()) return;

    if (USFMiniMapIcon** FoundIcon = TargetIcons.Find(Target.GetObject()))
    {
        if (*FoundIcon)
        {
            (*FoundIcon)->RemoveFromParent();
        }
        TargetIcons.Remove(Target.GetObject());
    }
}

void USFMinimapWidget::UpdateIconPositions()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;

    APawn* PlayerPawn = PC->GetPawn();
    if (!PlayerPawn) return;

    // 1. 내 위치 (미니맵의 중심)
    FVector PlayerLoc = PlayerPawn->GetActorLocation();
    
    // PlayerPawn->GetActorRotation() 대신 카메라 매니저
    FRotator ViewRot = FRotator::ZeroRotator;
    if (PC->PlayerCameraManager)
    {
        ViewRot = PC->PlayerCameraManager->GetCameraRotation();
    }
    else
    {
        // 예외적으로 카메라 매니저가 없을 때만 폰 회전 사용
        ViewRot = PlayerPawn->GetActorRotation();
    }
    
    for (auto It = TargetIcons.CreateIterator(); It; ++It)
    {
        UObject* TargetObj = It->Key;
        USFMiniMapIcon* IconWidget = It->Value;

        if (!IsValid(TargetObj) || !IsValid(IconWidget)) continue;
        
        FVector TargetLoc = ISFMiniMapTrackable::Execute_GetMiniMapWorldPosition(TargetObj);
        
        FVector RelativeLoc = TargetLoc - PlayerLoc;
        
        FVector UnrotatedLoc = RelativeLoc.RotateAngleAxis(-ViewRot.Yaw, FVector::UpVector);
        
        FVector2D FinalPos;
        
        FinalPos.X = UnrotatedLoc.Y / MiniMapScale; 
        
        FinalPos.Y = -UnrotatedLoc.X / MiniMapScale;
        
        if (FinalPos.Size() > MapRadius)
        {
            FinalPos = FinalPos.GetSafeNormal() * MapRadius;
            
            
            IconWidget->SetRenderOpacity(0.5f);
        }
        else
        {
            IconWidget->SetRenderOpacity(1.0f);
        }
        
        IconWidget->SetRenderTranslation(FinalPos);
    }
}