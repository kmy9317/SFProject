#include "Character/Enemy/Component/SFDragonMovementComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemGlobals.h" 
#include "AbilitySystemComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Boss_Dragon/SFDragonGameplayTags.h"
#include "Character/SFCharacterGameplayTags.h" 
#include "Character/SFPawnExtensionComponent.h"

USFDragonMovementComponent::USFDragonMovementComponent()
{
    MaxAcceleration = HeavyMaxAcceleration;          
    BrakingDecelerationWalking = HeavyBrakingDeceleration; 
    
    BrakingDecelerationFlying = 3000.0f; 
    
    GroundFriction = 8.0f; 

    RotationRate = FRotator(0.0f, 120.0f, 0.0f);
    bOrientRotationToMovement = true;
  
    bEnablePhysicsInteraction = false; 
    Mass = 50000.0f; 
    AirControl = 0.2f; 
    
    bIsSprinting = false;
    
    bUseRVOAvoidance = false; 
    AvoidanceWeight = 0.0f; 
    // AvoidanceConsiderationRadius = 800.0f; 
}

void USFDragonMovementComponent::InitializeMovementComponent()
{
    Super::InitializeMovementComponent();
    
    bUseRVOAvoidance = false;
    
}

float USFDragonMovementComponent::GetMaxSpeed() const
{
    if (IsFlying())
    {
       return DragonFlySpeed;
    }
    
    if (bIsSprinting)
    {
       return DragonRunSpeed;
    }
    
    return DragonWalkSpeed;
}

void USFDragonMovementComponent::SetSprinting(bool bNewSprinting)
{
    bIsSprinting = bNewSprinting;
}

void USFDragonMovementComponent::SetFlyingMode(bool bFly)
{
    
    if (!GetOwner()->HasAuthority())
    {
       
        return;
    }
    
    
    if (bFly)
    {
        SetMovementMode(MOVE_Flying);
    }
    else
    {
        SetMovementMode(MOVE_Walking);
    }

}

void USFDragonMovementComponent::InternalDisableMovement()
{
    StopMovementImmediately();
    if (IsFlying())
    {
        SetFlyingMode(false); 
    }
}

