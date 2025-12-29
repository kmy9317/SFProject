// Fill out your copyright notice in the Description page of Project Settings.


#include "BossArenaTrigger.h"

#include "Components/BoxComponent.h"
#include "AI/Controller/SFEnemyController.h"
#include "AI/Controller/Dragon/SFDragonCombatComponent.h"
#include "Character/SFCharacterBase.h"
#include "Team/SFTeamTypes.h"


// Sets default values
ABossArenaTrigger::ABossArenaTrigger()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(500.f, 500.f, 200.f));
	
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldStatic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
}

void ABossArenaTrigger::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{

		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ABossArenaTrigger::OnPlayerEnterArena);

		if (BossActor)
		{
			if (ASFBaseAIController* AIC = Cast<ASFBaseAIController>(BossActor->GetController()))
			{
				CachedDragonCombatComponent = Cast<USFDragonCombatComponent>(AIC->GetCombatComponent());

				if (!CachedDragonCombatComponent)
				{
					UE_LOG(LogTemp, Error, TEXT("[BossArenaTrigger] Failed to cache DragonCombatComponent! Boss: %s"),
						*BossActor->GetName());
				}
				
			}
			
		}
	
	}
}

void ABossArenaTrigger::OnPlayerEnterArena(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!CachedDragonCombatComponent)
	{
		return;
	}

	
	ASFCharacterBase* Character = Cast<ASFCharacterBase>(OtherActor);
	if (!Character)
	{
		return;
	}
	
	if (Character->GetGenericTeamId() != FGenericTeamId(SFTeamID::Player))
	{
		return;
	}

	if (PlayersInArena.Contains(Character))
	{
		return;
	}

	
	PlayersInArena.Add(Character);


	CachedDragonCombatComponent->AddThreat(InitialThreatValue, Character);
	CachedDragonCombatComponent->UpdateTargetFromThreat();
	
}

