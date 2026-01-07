#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "SFAN_WeaponBurst.generated.h"

class USFDA_WeaponBurstData;

/**
 * 애니메이션 몽타주에서 지정한 GameplayCue를 실행하며,
 * 설정된 WeaponBurstData를 파라미터(SourceObject)로 전달하는 노티파이
 */
UCLASS()
class SF_API USFAN_WeaponBurst : public UAnimNotify
{
	GENERATED_BODY()

public:
	USFAN_WeaponBurst();

	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	// 실행할 게임플레이 큐 태그 (예: GameplayCue.Weapon.Burst)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameplayCue")
	FGameplayTag GameplayCueTag;

	// 큐에 전달할 데이터 에셋 (VFX, SFX, 소켓 정보 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameplayCue")
	TObjectPtr<USFDA_WeaponBurstData> BurstData;
};