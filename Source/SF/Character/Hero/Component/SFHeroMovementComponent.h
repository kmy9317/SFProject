#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SFHeroMovementComponent.generated.h"

UENUM(BlueprintType)
enum class ESFSlidingMode : uint8
{
	None,
	// 기본 슬라이딩 동작
	Normal,
	
	// 슬라이딩 없이 정지
	StopOnHit,
	
	// 충돌 무시하고 관통
	PassThrough
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFHeroMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	// 충돌시 슬라이드 설정 (충돌 Response도 함께 처리)
	UFUNCTION(BlueprintCallable, Category = "SF|Movement")
	void SetSlidingMode(ESFSlidingMode NewMode);
	
protected:
	virtual float SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact) override;

private:
	void ApplyPassThroughCollision();
	void RestoreCollision();

public:

	UPROPERTY(BlueprintReadWrite, Category = "SF|Movement")
	ESFSlidingMode SlidingMode = ESFSlidingMode::Normal;

	// 정면 충돌 판정 각도 (이 각도 이내면 정면 충돌로 판정)
	// 0.5 = 약 60도, 0.7 = 약 45도, 0.85 = 약 30도
	UPROPERTY(EditDefaultsOnly, Category = "SF|Movement")
	float FrontalHitThreshold = 0.7f;
	
	// 슬라이딩 비활성화 (true면 충돌 시 정지)
	UPROPERTY(BlueprintReadWrite, Category = "SF|Movement")
	bool bDisableSliding = false;

	// 관통 모드에서 무시할 충돌 채널들
	UPROPERTY(EditDefaultsOnly, Category = "SF|Movement")
	TArray<TEnumAsByte<ECollisionChannel>> PassThroughChannels;

private:
	// 원본 Collision 정보 저장
	TMap<ECollisionChannel, ECollisionResponse> SavedCollisionResponses;
};
