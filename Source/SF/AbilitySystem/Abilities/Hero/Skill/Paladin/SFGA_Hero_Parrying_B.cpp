#include "AbilitySystem/Abilities/Hero/Skill/Paladin/SFGA_Hero_Parrying_B.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/Hero/Skill/SkillActor/SFBuffArea.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"


void USFGA_Hero_Parrying_B::OnParryEventReceived(FGameplayEventData Payload)
{
	Super::OnParryEventReceived(Payload);

	//===============이유는 모르겠지만 한번 더 해야 제대로 판정됨==================
	AActor* OwnerActor = GetAvatarActorFromActorInfo();
	if (!OwnerActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] OwnerActor NULL"));
		return;
	}

	// 1) 공격자 찾기
	AActor* InstigatorActor = Payload.ContextHandle.GetOriginalInstigator();
	if (!InstigatorActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] Failed GetOriginalInstigator"));
		InstigatorActor = const_cast<AActor*>(Payload.Instigator.Get());
	}

	if (!InstigatorActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] Instigator NULL - Cannot calculate angle"));
		return;
	}

	//방향 계산
	FVector OwnerLocation = OwnerActor->GetActorLocation();
	FVector InstigatorLocation = InstigatorActor->GetActorLocation();
	FVector DirToInstigator = (InstigatorLocation - OwnerLocation).GetSafeNormal();

	FVector OwnerForward = OwnerActor->GetActorForwardVector().GetSafeNormal();

	//각도 계산
	float Dot = FVector::DotProduct(OwnerForward, DirToInstigator);
	float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));

	//전방 180° 에서 공격이 들어왔는지 판단
	bool bInFront = (AngleDeg <= 90.0f);

	UE_LOG(LogTemp, Warning, TEXT("[Parry] Angle=%.2f | Dot=%.2f | InFront(180deg)=%d"),
		AngleDeg, Dot, bInFront);

	// 4) 후방 공격 → 패링 실패
	if (!bInFront)
	{
		UE_LOG(LogTemp, Error, TEXT("[Parry] FAILED - Attack from BEHIND"));
		return;
	}
	//========================================================================
	
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}
	
	if (!BuffAreaClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !CurrentActorInfo)
	{
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return;
	}
	
	//BuffArea 액터 소환 위치 지정
	FVector SpawnLoc = Avatar->GetActorLocation();

	float HalfHeight = 0.f;

	//Pawn인지 확인 후 Capsule 정보 가져오기
	if (ACharacter* Char = Cast<ACharacter>(Avatar))
	{
		if (auto* Capsule = Char->GetCapsuleComponent())
		{
			HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	//캐릭터 중심 → 발바닥 위치로 이동
	SpawnLoc.Z -= HalfHeight;

	//FX 추가 위치 보정
	SpawnLoc.Z += 3.f;
	
	//BuffArea 액터 소환
	FActorSpawnParameters Params;
	Params.Owner = Avatar;
	Params.Instigator = Cast<APawn>(Avatar);

	ASFBuffArea* AreaActor = World->SpawnActor<ASFBuffArea>(
		BuffAreaClass,
		SpawnLoc,
		FRotator::ZeroRotator,
		Params
	);

	if (AreaActor)
	{
		AreaActor->InitializeArea(ASC);
	}
}
