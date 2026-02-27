#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Hero_Parrying.h"
#include "SFGA_Hero_Parrying_B.generated.h"

class ASFBuffArea;

UCLASS()
class SF_API USFGA_Hero_Parrying_B : public USFGA_Hero_Parrying
{
	GENERATED_BODY()

protected:

	virtual void OnParrySuccess(const FGameplayEventData& Payload, AActor* InstigatorActor) override;
	
	//이 스킬이 소환할 장판 액터 클래스 (BP_SFBuffArea 지정)
	UPROPERTY(EditDefaultsOnly, Category="SF|Area")
	TSubclassOf<ASFBuffArea> BuffAreaClass;
};
