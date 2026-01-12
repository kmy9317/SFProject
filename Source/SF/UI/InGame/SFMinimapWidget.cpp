#include "SFMinimapWidget.h"
#include "SFMiniMapIcon.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "System/SFMinimapSubsystem.h"

void USFMinimapWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (IsDesignTime())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        if (APlayerController* PC = GetOwningPlayer())
        {
            World = PC->GetWorld();
        }
    }

    if (World)
    {
        MiniMapSubsystem = World->GetSubsystem<USFMinimapSubsystem>();
        if (MiniMapSubsystem)
        {
            MiniMapSubsystem->OnTargetRegistered.AddDynamic(this, &USFMinimapWidget::OnTargetRegistered);
            MiniMapSubsystem->OnTargetUnregistered.AddDynamic(this, &USFMinimapWidget::OnTargetUnregistered);
            
            const TArray<TScriptInterface<ISFMiniMapTrackable>>& ExistingTargets = MiniMapSubsystem->GetAllTargets();
            
            for (const auto& Target : ExistingTargets)
            {
                OnTargetRegistered(Target);
            }
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
        
        UPanelSlot* PanelSlot = MiniMapCanvas->AddChild(NewIcon);
        
        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(PanelSlot))
        {
            CanvasSlot->SetAnchors(FAnchors(0.5f));
            CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            CanvasSlot->SetAutoSize(true);
            CanvasSlot->SetPosition(FVector2D::ZeroVector);
            CanvasSlot->SetZOrder(10);
        }

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

    FVector PlayerLoc = PlayerPawn->GetActorLocation();
    
    FRotator ViewRot = FRotator::ZeroRotator;
    if (PC->PlayerCameraManager)
    {
        ViewRot = PC->PlayerCameraManager->GetCameraRotation();
    }
    else
    {
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