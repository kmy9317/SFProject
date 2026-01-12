#include "SFMiniMapIcon.h"
#include "Components/Image.h"
#include "Interface/SFMiniMapTrackable.h"
#include "GameFramework/Character.h" 
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Character/SFCharacterGameplayTags.h"

void USFMiniMapIcon::SetTarget(TScriptInterface<ISFMiniMapTrackable> InTarget)
{
    TrackedTarget = InTarget;

    if (!TrackedTarget.GetObject() || !IconImage) return;

    UObject* TargetObj = TrackedTarget.GetObject();
    
    EMiniMapIconType Type = ISFMiniMapTrackable::Execute_GetMiniMapIconType(TargetObj);
    
    if (UTexture2D** FoundTexture = IconTextures.Find(Type))
    {
       IconImage->SetBrushFromTexture(*FoundTexture);
    }
    
    float Scale = (Type == EMiniMapIconType::Boss) ? BossSizeMultiplier : 1.0f;
    SetRenderScale(FVector2D(Scale, Scale));
    
    FLinearColor Color = FLinearColor::White;
    switch (Type)
    {
        case EMiniMapIconType::Player: 
            Color = PlayerColor; 
            break;
        case EMiniMapIconType::Enemy:  Color = EnemyColor; break;
        case EMiniMapIconType::Boss:   Color = BossColor; break;
        default: break;
    }
    IconImage->SetColorAndOpacity(Color);
}

void USFMiniMapIcon::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!TrackedTarget.GetObject() || !IconImage) return;


    
    if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TrackedTarget.GetObject()))
    {
        if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
        {
             if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Downed))
             {
                 BlinkTimer += InDeltaTime;
                 if (BlinkTimer >= 0.5f)
                 {
                    BlinkTimer = 0.f;
                    bBlinkState = !bBlinkState;
                    IconImage->SetOpacity(bBlinkState ? 1.0f : 0.3f);
                 }
                 return;
             }
        }
    }
    
    if (IconImage->GetRenderOpacity() != 1.0f)
    {
        IconImage->SetOpacity(1.0f);
    }
}