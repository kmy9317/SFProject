#include "SFAnimNotifyState_BlockMovement.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"

void USFAnimNotifyState_BlockMovement::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner()))
	{
		// 이동 입력 무시 시작 (WASD가 안 먹힘)
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			PC->SetIgnoreMoveInput(true);
		}
	}
}

void USFAnimNotifyState_BlockMovement::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner()))
	{
		// 이동 입력 무시 해제 (이제 움직일 수 있음)
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			PC->SetIgnoreMoveInput(false);
		}
	}
}