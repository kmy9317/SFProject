#include "SFDragonFireballProjectile.h"
#include "NiagaraComponent.h"          // [추가] 컴포넌트 접근용
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayCueFunctionLibrary.h"
#include "GenericTeamAgentInterface.h"
#include "AbilitySystem/GameplayCues/SFGameplayCueTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterBase.h"

ASFDragonFireballProjectile::ASFDragonFireballProjectile()
{
    InitialSpeed = 1500.f;
    MaxSpeed = 2000.f;
    GravityScale = 0.0f;
}


void ASFDragonFireballProjectile::BeginPlay()
{
    Super::BeginPlay();

    if (ProjectileEffect && DefaultNiagaraEffect)
    {
        ProjectileEffect->SetAsset(DefaultNiagaraEffect);
        ProjectileEffect->Activate(true);
    }
}

void ASFDragonFireballProjectile::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

    if (!OtherActor || OtherActor == this || OtherActor == GetOwner())
    {
        return;
    }

    if (!OwnerChar)
    {
        if (HasAuthority()) Destroy();
        return;
    }
    
    
    if (HasAuthority())
    {
     
        bool bIsHostile = false;
        if (OtherActor->Implements<UGenericTeamAgentInterface>())
        {
            const ETeamAttitude::Type Attitude = OwnerChar->GetTeamAttitudeTowards(*OtherActor);
            bIsHostile = (Attitude == ETeamAttitude::Hostile);
        }

        if (bIsHostile && DamageEffectClass)
        {
            UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
            UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerChar);

            if (TargetASC && SourceASC)
            {
                FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
                ContextHandle.AddHitResult(Hit);
                ContextHandle.AddInstigator(OwnerChar, OwnerChar);
                ContextHandle.AddSourceObject(this);

                FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);

                if (SpecHandle.IsValid())
                {
                    SpecHandle.Data.Get()->SetSetByCallerMagnitude(
                        SFGameplayTags::Data_Damage_BaseDamage,
                        BaseDamage
                    );

                    SourceASC->ApplyGameplayEffectSpecToTarget(
                        *SpecHandle.Data.Get(),
                        TargetASC
                    );
                }
            }
        }
        
        FGameplayCueParameters CueParams;
        CueParams.Location = Hit.ImpactPoint; 
        CueParams.Normal = Hit.ImpactNormal;
        CueParams.Instigator = OwnerChar;
        CueParams.EffectCauser = this;
        
        UGameplayCueFunctionLibrary::ExecuteGameplayCueOnActor(
            OwnerChar, 
            SFGameplayTags::GameplayCue_Dragon_FireBallExplosion,
            CueParams
        );
        Destroy();
    }
}