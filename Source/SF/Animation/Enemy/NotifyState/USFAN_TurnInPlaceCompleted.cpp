// Fill out your copyright notice in the Description page of Project Settings.


#include "USFAN_TurnInPlaceCompleted.h"

#include "AIController.h"
#include "AI/Controller/SFBaseAIController.h"
#include "Animation/Enemy/SFEnemyAnimInstance.h"

void UUSFAN_TurnInPlaceCompleted::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                         const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (!MeshComp) return;

	APawn* Pawn = Cast<APawn>(MeshComp->GetOwner());
	if (!Pawn) return;
	
	USFEnemyAnimInstance* AnimInstance = Cast<USFEnemyAnimInstance>(MeshComp->GetAnimInstance());
	if (!AnimInstance)
	{
		return;
	}


	
}
