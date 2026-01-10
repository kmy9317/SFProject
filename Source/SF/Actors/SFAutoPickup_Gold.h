#pragma once

#include "CoreMinimal.h"
#include "SFAutoPickup.h"
#include "SFAutoPickup_Gold.generated.h"

UCLASS()
class SF_API ASFAutoPickup_Gold : public ASFAutoPickup
{
	GENERATED_BODY()

protected:
	virtual void ApplyCollectEffect(ASFPlayerState* PlayerState, int32 CollectAmount) override;
};
