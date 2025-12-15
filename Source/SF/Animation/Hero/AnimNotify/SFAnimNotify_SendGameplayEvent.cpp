#include "SFAnimNotify_SendGameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"

USFAnimNotify_SendGameplayEvent::USFAnimNotify_SendGameplayEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	bShouldFireInEditor = false;
#endif
	bIsNativeBranchingPoint = true;
}

void USFAnimNotify_SendGameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (EventTag.IsValid())
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MeshComp->GetOwner(), EventTag, EventData);
	}
}
