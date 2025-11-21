// SFAnimNotifyState_AttackWindow.cpp
#include "Animation/Enemy/NotifyState/SFAnimNotifyState_AttackWindow.h"

#include "AI/SFAIGameplayTags.h"
#include "Equipment/SFEquipmentTags.h"
#include "Interface/SFTraceActorInterface.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "Equipment/EquipmentInstance/SFEquipmentInstance.h"
#include "System/SFInitGameplayTags.h"
#include "Weapons/Actor/SFMeleeWeaponActor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFAnimNotifyState_AttackWindow)


void USFAnimNotifyState_AttackWindow::FindWeapons(USkeletalMeshComponent* MeshComp)
{
    CachedWeapons.Empty();
    
    if (!MeshComp)
        return;
    
    AActor* Owner = MeshComp->GetOwner();
    if (!Owner)
        return;
    

    USFEquipmentComponent* EquipmentComp = USFEquipmentComponent::FindEquipmentComponent(Owner);
    if (!EquipmentComp)
        return;
    
    const TArray<USFEquipmentInstance*>& EquippedItems = EquipmentComp->GetEquippedItems();
    for (USFEquipmentInstance* Instance : EquippedItems)
    {
        if (!Instance)
            continue;
        USFEquipmentDefinition* EquipmentDefinition = Instance->GetEquipmentDefinition();
        if (!EquipmentDefinition)
            continue;
        if (!EquipmentDefinition->EquipmentTag.MatchesTag(SFGameplayTags::EquipmentTag_Weapon))
            continue;
        
        const TArray<AActor*>& SpawnedActors = Instance->GetSpawnedActors();
        for (AActor* Actor : SpawnedActors)
        {
            if (ISFTraceActorInterface* TraceActor = Cast<ISFTraceActorInterface>(Actor))
            {

                if (AllowedSocketNames.Num() >0 )
                {
                    FName AttachSocket = Actor->GetAttachParentSocketName();
                    if (!AllowedSocketNames.Contains(AttachSocket))
                        continue;
                }
                CachedWeapons.Add(Actor);
            }
        }
    }
}

void USFAnimNotifyState_AttackWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
    
    if (!MeshComp)
        return;
    
    AActor* Owner = MeshComp->GetOwner();
    if (!Owner)
        return;
    
    FindWeapons(MeshComp);
    
    for (AActor* WeaponActor : CachedWeapons)
    {
        if (ISFTraceActorInterface* TraceActor = Cast<ISFTraceActorInterface>(WeaponActor))
        {
            TraceActor->OnTraceStart(Owner);
        }
    }
}


void USFAnimNotifyState_AttackWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

    // Actor Tick에서 자동으로 PerformTrace가 호출되므로 여기서는 아무것도 하지 않음
}

void USFAnimNotifyState_AttackWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);
    
    if (!MeshComp)
        return;
    
    AActor* Owner = MeshComp->GetOwner();
    if (!Owner)
        return;
    
    for (AActor* WeaponActor : CachedWeapons)
    {
        if (ISFTraceActorInterface* TraceActor = Cast<ISFTraceActorInterface>(WeaponActor))
        {
            TraceActor->OnTraceEnd(Owner);
        }
    }
    
    CachedWeapons.Empty();
}