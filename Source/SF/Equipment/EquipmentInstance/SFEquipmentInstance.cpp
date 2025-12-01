// Fill out your copyright notice in the Description page of Project Settings.


#include "SFEquipmentInstance.h"
#include "Equipment/SFEquipmentDefinition.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimInstance.h"
#include "AbilitySystem/Abilities/Enemy/SFEnemyAbilityInitializer.h"
#include "Character/Enemy/SFEnemy.h"
#include "System/SFGameInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFEquipmentInstance)

USFEquipmentInstance::USFEquipmentInstance(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void USFEquipmentInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, EquipmentDefinition);
    DOREPLIFETIME(ThisClass, Instigator);
    DOREPLIFETIME(ThisClass, SpawnedActors);
}

void USFEquipmentInstance::Initialize(USFEquipmentDefinition* InDefinition, APawn* InPawn, UAbilitySystemComponent* ASC)
{
    if (!InDefinition || !InPawn)
    {
        return;
    }

    EquipmentDefinition = InDefinition;
    Instigator = InPawn;

    SpawnEquipmentActors();
    
    if (ASC)
    {
        GrantAbilities(ASC);
    }

    // Listen Server 로직
    ApplyAnimationLayer();
}

void USFEquipmentInstance::SpawnEquipmentActors()
{
    if (!EquipmentDefinition || !Instigator)
    {
        return;
    }

    UWorld* World = Instigator->GetWorld();
    if (!World)
    {
        return;
    }
    
    const FSFEquipmentEntry& Entry = EquipmentDefinition->EquipmentEntry;
    
    USkeletalMeshComponent* PawnMesh = Instigator->FindComponentByClass<USkeletalMeshComponent>();
    if (!PawnMesh)
    {
       return;
    }

    //Spawn할 Actor 가져와서 스폰해버림 
    for (const FSFEquipmentActorToSpawn& SpawnInfo : Entry.ActorsToSpawn)
    {
        if (!SpawnInfo.ActorToSpawn)
        {
            continue;
        }
        
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = Instigator;                    // Owner 설정
        SpawnParams.Instigator = Instigator;               // Instigator 설정
        SpawnParams.SpawnCollisionHandlingOverride = 
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;  // 항상 스폰

        // 이거 Spawn을 하는데 만들어두고 Deffer로 할까 고민 중 왜냐하면 Finish스폰 전에 Ability를 줄까 고민 중~ 
        AActor* SpawnedActor = World->SpawnActor<AActor>(
            SpawnInfo.ActorToSpawn,
            FVector::ZeroVector,      
            FRotator::ZeroRotator,    
            SpawnParams
        );

        if (!SpawnedActor)
        {
            continue;
        }

        // Pawn의 mesh에 Attach
        if (PawnMesh)
        {
            if (SpawnInfo.AttachSocket == NAME_None || 
                !PawnMesh->DoesSocketExist(SpawnInfo.AttachSocket))
            {
                SpawnedActor->Destroy();
                continue;
            }
            
            SpawnedActor->AttachToComponent(
                PawnMesh,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,  
                SpawnInfo.AttachSocket
            );
            
            SpawnedActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);
            SpawnedActors.Add(SpawnedActor);
        }
    }
}

void USFEquipmentInstance::DestroyEquipmentActors()
{
    for (AActor* Actor : SpawnedActors)
    {
        if (Actor && !Actor->IsPendingKillPending())
        {
            Actor->Destroy();
        }
    }
    SpawnedActors.Empty();
}

void USFEquipmentInstance::GrantAbilities(UAbilitySystemComponent* ASC)
{
    if (!ASC || !EquipmentDefinition)
    {
        return;
    }

    AActor* Owner = ASC->GetOwner();
    if (!Owner)
    {
        return;
    }

    const bool bIsEnemy = Owner->IsA<ASFEnemy>();

    USFGameInstance* GI = nullptr;
    if (bIsEnemy)
    {
        GI = Cast<USFGameInstance>(Owner->GetWorld()->GetGameInstance());
    }

    for (const TSubclassOf<USFGameplayAbility>& AbilityClass : EquipmentDefinition->GrantedAbilities)
    {
        if (!AbilityClass)
        {
            continue;
        }

        FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);

   
        FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
        if (!Handle.IsValid())
        {
            continue;
        }

        // 적일 경우에만 AbilityData 적용 
        if (bIsEnemy && GI)
        {
            USFGameplayAbility* CDO = Cast<USFGameplayAbility>(AbilityClass->GetDefaultObject());
            if (CDO)
            {
                FName AbilityID = CDO->GetAbilityID();
                if (!AbilityID.IsNone())
                {
                    if (const FAbilityBaseData* Data = GI->FindAbilityData(AbilityID))
                    {
                        FGameplayAbilitySpec* RealSpec = ASC->FindAbilitySpecFromHandle(Handle);
                        if (RealSpec)
                        {
                            USFEnemyAbilityInitializer::ApplyAbilityData(*RealSpec, *Data);
                        }
                    }
                }
            }
        }

        GrantedAbilityHandles.Add(Handle);
    }
}

void USFEquipmentInstance::RemoveAbilities(UAbilitySystemComponent* ASC)
{
    if (ASC)
    {
        for (const FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
        {
            if (Handle.IsValid())
            {
                ASC->ClearAbility(Handle);
            }
        }
        GrantedAbilityHandles.Empty();

        for (const FActiveGameplayEffectHandle& Handle : GrantedEffectHandles)
        {
            if (Handle.IsValid())
            {
                ASC->RemoveActiveGameplayEffect(Handle);
            }
        }
        GrantedEffectHandles.Empty();
    }
}

void USFEquipmentInstance::Deinitialize(UAbilitySystemComponent* ASC)
{
    // 어빌리티 제거 
    if (ASC)
    {
        RemoveAbilities(ASC);
    }

    // Listen Server 로직
    RemoveAnimationLayer();

    // 스폰된 Actor 제거
    DestroyEquipmentActors();
    
    //참조 정리
    EquipmentDefinition = nullptr;
    Instigator = nullptr;
    
}

void USFEquipmentInstance::ApplyAnimationLayer()
{
    if (!EquipmentDefinition || !Instigator)
    {
        return;
    }

    USkeletalMeshComponent* PawnMesh = Instigator->FindComponentByClass<USkeletalMeshComponent>();
    if (!PawnMesh)
    {
        return;
    }
    TSubclassOf<UAnimInstance> AnimLayerInfo = EquipmentDefinition->AnimLayerInfo;
    if (!IsValid(AnimLayerInfo))
    {
        return;
    }

    // 이미 링크되어 있는지 확인
    if (PawnMesh->GetLinkedAnimLayerInstanceByClass(AnimLayerInfo) == nullptr)
    {
        // Animation Layer 적용
        PawnMesh->LinkAnimClassLayers(AnimLayerInfo);
    }
}

void USFEquipmentInstance::RemoveAnimationLayer()
{
    if (!EquipmentDefinition || !Instigator)
    {
        return;
    }

    USkeletalMeshComponent* PawnMesh = Instigator->FindComponentByClass<USkeletalMeshComponent>();
    if (!PawnMesh)
    {
        return;
    }

    TSubclassOf<UAnimInstance> AnimLayerInfo = EquipmentDefinition->AnimLayerInfo;
    if (!IsValid(AnimLayerInfo))
    {
        return;
    }

    // Animation Layer 제거
    PawnMesh->UnlinkAnimClassLayers(AnimLayerInfo);
    
}

void USFEquipmentInstance::OnEquipped()
{
    ApplyAnimationLayer();
}

void USFEquipmentInstance::OnUnequipped()
{
    RemoveAnimationLayer();
}

void USFEquipmentInstance::OnRep_EquipmentDefinition()
{
    OnEquipped();
}

void USFEquipmentInstance::OnRep_Instigator()
{
    OnEquipped();
}
