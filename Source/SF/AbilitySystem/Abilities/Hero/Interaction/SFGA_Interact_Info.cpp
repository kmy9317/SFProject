#include "SFGA_Interact_Info.h"

#include "Interaction/SFInteractable.h"
#include "Interaction/SFInteractionQuery.h"

USFGA_Interact_Info::USFGA_Interact_Info(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
    
}

bool USFGA_Interact_Info::InitializeAbility(AActor* TargetActor)
{
	TScriptInterface<ISFInteractable> TargetInteractable(TargetActor);
	if (TargetInteractable)
	{
		FSFInteractionQuery InteractionQuery;
		InteractionQuery.RequestingAvatar = GetAvatarActorFromActorInfo();
		InteractionQuery.RequestingController = GetControllerFromActorInfo();

		Interactable = TargetInteractable;
		InteractableActor = TargetActor;

		TArray<FSFInteractionInfo> InteractionInfos;
		FSFInteractionInfoBuilder InteractionInfoBuilder(Interactable, InteractionInfos);
		Interactable->GatherPostInteractionInfos(InteractionQuery, InteractionInfoBuilder);
		InteractionInfo = InteractionInfos[0];

		return true;
	}

	return false;
}
