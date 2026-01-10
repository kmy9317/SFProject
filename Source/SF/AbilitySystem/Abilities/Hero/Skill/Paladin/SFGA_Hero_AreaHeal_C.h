#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Skill_Melee.h"
#include "GameplayTagContainer.h"
#include "SFGA_Hero_AreaHeal_C.generated.h"

class UGameplayEffect;
class UAnimMontage;
class UNiagaraSystem;
class UNiagaraComponent;
class USkeletalMeshComponent;

UCLASS()
class SF_API USFGA_Hero_AreaHeal_C : public USFGA_Skill_Melee
{
	GENERATED_BODY()

public:
	USFGA_Hero_AreaHeal_C(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//=====================Ability Lifecycle============================
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;
	//===================================================================

	//=====================이벤트 수신 (번개 시퀀스 시작)===============
	UFUNCTION()
	void OnLightningImpact(FGameplayEventData Payload);

	// 실제 번개 1회를 생성하고 다음 타이머를 예약하는 함수
	void ExecuteSingleLightning();
	//===================================================================

	UFUNCTION()
	void OnMontageEnded();

	//=====================현재 장착 무기 메쉬 찾기=======================
	USkeletalMeshComponent* FindCurrentWeaponMesh(class ASFCharacterBase* OwnerChar) const;
	//===================================================================

protected:

	//=====================Gameplay Tags===============================
	UPROPERTY(EditAnywhere, Category="SF|Tags")
	FGameplayTag LightningCueTag; //FX 호출용 GC 태그

	UPROPERTY(EditAnywhere, Category="SF|Tags")
	FGameplayTag LightningEventTag;	//실제 로직 수행 AnimNotify GameplayEvent 태그
	//==================================================================

	//=====================연속 소환(Cascade) 설정=======================
	UPROPERTY(EditAnywhere, Category="SF|Gameplay|Cascade")
	int32 MaxStrikeCount = 5; // 총 소환할 번개 개수

	UPROPERTY(EditAnywhere, Category="SF|Gameplay|Cascade")
	float StrikeInterval = 0.1f; // 번개 소환 간격 (초)

	UPROPERTY(EditAnywhere, Category="SF|Gameplay|Cascade")
	float StrikeBaseDistance = 300.f; // 첫 번째 번개가 소환될 거리

	UPROPERTY(EditAnywhere, Category="SF|Gameplay|Cascade")
	float StrikeSpacing = 150.f; // 번개 간의 거리 간격 (n만큼 앞에 소환)
	//==================================================================

	UPROPERTY(EditAnywhere, Category="SF|Gameplay")
	float StrikeRadius = 200.f; //공격 범위

	UPROPERTY(EditAnywhere, Category="SF|Gameplay")
	TSubclassOf<UGameplayEffect> DebuffGE; //디버프 GE 적용
	//==================================================================

	//=====================Animation===============================
	UPROPERTY(EditAnywhere, Category="SF|Animation")
	UAnimMontage* LightningMontage;
	//==================================================================

	//=====================Trail FX===============================
	UPROPERTY(EditDefaultsOnly, Category="SF|VFX")
	UNiagaraSystem* SwordTrailFX;

	UPROPERTY()
	UNiagaraComponent* TrailComp;

	FTimerHandle TrailUpdateHandle;
	FTimerHandle TrailFadeHandle;
	//==================================================================

private:
	// 내부 로직용 변수
	FTimerHandle CascadeTimerHandle; // 연속 소환 타이머
	int32 CurrentStrikeIndex = 0;    // 현재 몇 번째 소환인지 체크
	FVector CachedOriginLocation;    // 스킬 시전 시점의 캐릭터 위치
	FVector CachedForwardVector;     // 스킬 시전 시점의 캐릭터 방향
};