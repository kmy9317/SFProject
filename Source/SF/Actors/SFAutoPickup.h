#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item/SFItemInstance.h"
#include "SFAutoPickup.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class USFItemInstance;
class USFItemFragment_AutoPickup;
class ASFPlayerState;

UENUM(BlueprintType)
enum class ESFAutoPickupState : uint8
{
    Waiting,    // 감지 대기 중
    Idle,       // 감지 가능
    Homing,     // 플레이어 추적 중
    Collected   // 수집됨
};

UCLASS()
class SF_API ASFAutoPickup : public AActor
{
	GENERATED_BODY()

public:
    ASFAutoPickup();

    void Initialize(USFItemInstance* InItemInstance, int32 InAmount);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void EnableOverlapDetection();
    void StartHoming(AActor* Target);

    UFUNCTION()
    void OnDetectionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnCollectionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    void Collect(AActor* Collector);
    bool IsPlayerCharacter(AActor* Actor) const;

    virtual void ApplyCollectEffect(ASFPlayerState* PlayerState, int32 CollectAmount) {}

    UFUNCTION()
    void OnRep_PickupState();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> CollisionSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> DetectionSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> CollectionSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

    UPROPERTY(ReplicatedUsing = OnRep_PickupState, BlueprintReadOnly, Category = "State")
    ESFAutoPickupState PickupState = ESFAutoPickupState::Waiting;

    UPROPERTY()
    TObjectPtr<USFItemInstance> ItemInstance;

    UPROPERTY()
    TWeakObjectPtr<AActor> HomingTarget;

    UPROPERTY(BlueprintReadOnly, Category = "AutoPickup")
    int32 Amount = 1;

    UPROPERTY(BlueprintReadOnly, Category = "AutoPickup")
    bool bApplyToAllPlayers = true;

    // Fragment 설정값
    float DetectionRadius = 500.f;
    float CollectionRadius = 50.f;
    float HomingAcceleration = 3000.f;
    float MaxHomingSpeed = 1500.f;

    // 에디터 설정
    UPROPERTY(EditDefaultsOnly, Category = "AutoPickup")
    float ActivationDelay = 1.f;

    UPROPERTY(EditDefaultsOnly, Category = "Drop")
    float DropInitialSpeed = 400.f;

    UPROPERTY(EditDefaultsOnly, Category = "Drop")
    float DropInitialAngle = 60.f;

    // 런타임 상태
    float CurrentHomingSpeed = 0.f;

    FTimerHandle ActivationTimerHandle;
};
