#include "SFAbilityTask_WaitForInvalidInteraction.h"

#include "Character/SFCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

USFAbilityTask_WaitForInvalidInteraction::USFAbilityTask_WaitForInvalidInteraction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
    
}

USFAbilityTask_WaitForInvalidInteraction* USFAbilityTask_WaitForInvalidInteraction::WaitForInvalidInteraction(UGameplayAbility* OwningAbility, float AcceptanceAngle, float AcceptanceDistance)
{
	USFAbilityTask_WaitForInvalidInteraction* Task = NewAbilityTask<USFAbilityTask_WaitForInvalidInteraction>(OwningAbility);
	Task->AcceptanceAngle = AcceptanceAngle;
	Task->AcceptanceDistance = AcceptanceDistance;
	return Task;
}

void USFAbilityTask_WaitForInvalidInteraction::Activate()
{
	Super::Activate();

	SetWaitingOnAvatar();

	CachedCharacterForward2D = GetAvatarActor() ? GetAvatarActor()->GetActorForwardVector().GetSafeNormal2D() : FVector::ZeroVector;
	CachedCharacterLocation = GetAvatarActor() ? GetAvatarActor()->GetActorLocation() : FVector::ZeroVector;

	GetWorld()->GetTimerManager().SetTimer(CheckTimerHandle, this, &ThisClass::PerformCheck, 0.05f, true);
}

void USFAbilityTask_WaitForInvalidInteraction::OnDestroy(bool bInOwnerFinished)
{
	GetWorld()->GetTimerManager().ClearTimer(CheckTimerHandle);
	
	Super::OnDestroy(bInOwnerFinished);
}

void USFAbilityTask_WaitForInvalidInteraction::PerformCheck()
{
	ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(Ability->GetCurrentActorInfo()->AvatarActor.Get());
	UCharacterMovementComponent* CharacterMovement = SFCharacter->GetCharacterMovement();

	if (SFCharacter && CharacterMovement)
	{
		// 1. 각도 검사 (2D 평면에서의 회전)
		bool bValidAngle2D = CalculateAngle2D() <= AcceptanceAngle;
		
		// 2. 수평 거리 검사 (XY 평면에서의 이동)
		bool bValidDistanceXY = FVector::DistSquared2D(CachedCharacterLocation, SFCharacter->GetActorLocation()) <= (AcceptanceDistance * AcceptanceDistance);
		
		// 3. 수직 거리 검사 (Z축 이동, 앉기/서기 상태 고려)
		// 캐릭터의 앉기 상태 변화를 고려하여 허용 거리에 캡슐 높이 차이를 추가
		bool bValidDistanceZ = FMath::Abs(CachedCharacterLocation.Z - SFCharacter->GetActorLocation().Z) <= (AcceptanceDistance + CharacterMovement->GetCrouchedHalfHeight());

		if (bValidAngle2D && bValidDistanceXY && bValidDistanceZ)
		{
			return;
		}
	}

	// 하나라도 조건을 만족하지 않으면 Invalid 상호작용으로 판단
	OnInvalidInteraction.Broadcast();
	EndTask();
}

float USFAbilityTask_WaitForInvalidInteraction::CalculateAngle2D() const
{
	AActor* AvatarActor = Ability->GetCurrentActorInfo()->AvatarActor.Get();
	APlayerController* PlayerController = Ability->GetCurrentActorInfo()->PlayerController.Get();
	
	if (AvatarActor && PlayerController)
	{
		FVector CharacterForward2D = AvatarActor->GetActorForwardVector().GetSafeNormal2D();

		// 내적을 이용한 각도 계산 (코사인 법칙)
		// DegAcos: 아크코사인을 도 단위로 반환
		return UKismetMathLibrary::DegAcos(CachedCharacterForward2D.Dot(CharacterForward2D));
	}
	
	return 0.f;
}