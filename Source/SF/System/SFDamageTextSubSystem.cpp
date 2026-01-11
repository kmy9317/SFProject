#include "System/SFDamageTextSubSystem.h"
#include "SFDamageSettings.h"
#include "Components/WidgetComponent.h"
#include "UI/InGame/SFDamageWidget.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

void USFDamageTextSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (const USFDamageSettings* Settings = GetDefault<USFDamageSettings>())
    {
        DamageWidgetClass = Settings->DamageWidgetClass.LoadSynchronous();
    }
}

void USFDamageTextSubSystem::Deinitialize()
{
    for (auto& Pair : ActiveWidgetMap)
    {
        if (IsValid(Pair.Value))
        {
            Pair.Value->DestroyComponent();
        }
    }
    for (auto& Comp  : AvailablePool)
    {
        if (IsValid(Comp))
        {
            Comp->DestroyComponent();
        }
    }

    if (PoolOwnerActor)
    {
        PoolOwnerActor->Destroy();
    }

    Super::Deinitialize();
}

void USFDamageTextSubSystem::ShowDamage(float DamageAmount, AActor* TargetActor, FVector HitLocation, bool bIsCritical)
{
    if (!TargetActor || !DamageWidgetClass) return;

    UWidgetComponent* WidgetComp = GetFromPool();
    if (!WidgetComp) return;
    
    FVector SpawnLocation = CalcDamageTextLocation(TargetActor, HitLocation);
    
    SpawnLocation += FMath::VRand() * 10.f;

    WidgetComp->SetWorldLocation(SpawnLocation);
    WidgetComp->SetVisibility(true);

    if (USFDamageWidget* DamageWidget = Cast<USFDamageWidget>(WidgetComp->GetUserWidgetObject()))
    {
        ActiveWidgetMap.Add(DamageWidget, WidgetComp);
        DamageWidget->OnFinished.RemoveAll(this);
        DamageWidget->OnFinished.AddDynamic(this, &ThisClass::OnWidgetAnimationFinished);
        DamageWidget->PlayDamageEffect(DamageAmount, bIsCritical);
    }
}

UWidgetComponent* USFDamageTextSubSystem::GetFromPool()
{
    if (AvailablePool.Num() > 0)
    {
        return AvailablePool.Pop();
    }

    if (!PoolOwnerActor)
    {
        PoolOwnerActor = GetWorld()->SpawnActor<AActor>();
    }

    UWidgetComponent* NewComp = NewObject<UWidgetComponent>(PoolOwnerActor);
    NewComp->SetWidgetClass(DamageWidgetClass);
    NewComp->SetWidgetSpace(EWidgetSpace::Screen); 
    NewComp->SetDrawAtDesiredSize(true);
    NewComp->RegisterComponent();
    
    return NewComp;
}

void USFDamageTextSubSystem::OnWidgetAnimationFinished(UUserWidget* Widget)
{
    if (auto* FoundComp = ActiveWidgetMap.Find(Widget))
    {
        ReturnToPool(*FoundComp);
        ActiveWidgetMap.Remove(Widget);
    }
}

void USFDamageTextSubSystem::ReturnToPool(UWidgetComponent* Component)
{
    if (Component)
    {
        Component->SetVisibility(false);
        AvailablePool.Add(Component);
    }
}

FVector USFDamageTextSubSystem::CalcDamageTextLocation(AActor* TargetActor, FVector HitLocation)
{
    
    if (USkeletalMeshComponent* Mesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
    {
        if (Mesh->DoesSocketExist(TEXT("DamageText")))
        {
            return Mesh->GetSocketLocation(TEXT("DamageText"));
        }
    }
    if (!HitLocation.IsNearlyZero())
    {
        return HitLocation;
    }
    
    if (ACharacter* Char = Cast<ACharacter>(TargetActor))
    {
        return Char->GetActorLocation() + FVector(0.f, 0.f, Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    }
    
    return TargetActor->GetActorLocation();
}