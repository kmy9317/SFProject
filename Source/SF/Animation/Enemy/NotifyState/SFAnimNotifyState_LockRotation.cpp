#include "SFAnimNotifyState_LockRotation.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"

USFAnimNotifyState_LockRotation::USFAnimNotifyState_LockRotation()
{
#if WITH_EDITORONLY_DATA
    NotifyColor = FColor::Red;
#endif
}

void USFAnimNotifyState_LockRotation::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner()))
    {
        Character->bUseControllerRotationYaw = false;

        if (Character->GetCharacterMovement())
        {
            Character->GetCharacterMovement()->bUseControllerDesiredRotation = false;
            Character->GetCharacterMovement()->bOrientRotationToMovement = false;
        }
    }
}

void USFAnimNotifyState_LockRotation::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner()))
    {
        // [복구] 회전 기능 다시 켜기
        
        // 주의: 원래 BP 세팅이 bUseControllerRotationYaw = true 였다면 true로,
        // false(부드러운 회전)였다면 false로 둬야 합니다.
        // 보통 AI는 '부드러운 회전'을 위해 Pawn Yaw는 false, Movement Desired는 true를 씁니다.
        
        // 여기서는 일반적인 '부드러운 AI 회전' 세팅으로 복구합니다.
        Character->bUseControllerRotationYaw = false; 

        if (Character->GetCharacterMovement())
        {
            Character->GetCharacterMovement()->bUseControllerDesiredRotation = true;
            // 이동 방향 회전은 상황에 따라 다르지만 보통 전투 중엔 끕니다.
            // Character->GetCharacterMovement()->bOrientRotationToMovement = true; 
        }
    }
}