// Fill out your copyright notice in the Description page of Project Settings.


#include "SFMinimapSubsystem.h"

#include "Interface/SFMiniMapTrackable.h"

void USFMinimapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Targets.Empty();
}

void USFMinimapSubsystem::Deinitialize()
{
	Targets.Empty();
	Super::Deinitialize();
}

void USFMinimapSubsystem::RegisterTarget(TScriptInterface<ISFMiniMapTrackable> Target)
{
	Targets.Add(Target);
	OnTargetRegistered.Broadcast(Target);
}

void USFMinimapSubsystem::UnregisterTarget(TScriptInterface<ISFMiniMapTrackable> Target)
{
	if (!Target)
	{
		return;
	}

	int32 Removed = Targets.Remove(Target);
	if (Removed > 0)
	{
		OnTargetUnregistered.Broadcast(Target);

	}
}

TArray<TScriptInterface<ISFMiniMapTrackable>> USFMinimapSubsystem::GetTargetsByType(EMiniMapIconType Type) const
{
	TArray<TScriptInterface<ISFMiniMapTrackable>> Filtered;
	for (const auto& Target : Targets)
	{
		if (Target && Target->Execute_GetMiniMapIconType(Target.GetObject()) == Type)
		{
			Filtered.Add(Target);
		}
	}
	return Filtered;
}

const TArray<TScriptInterface<ISFMiniMapTrackable>>& USFMinimapSubsystem::GetAllTargets() const
{
	return Targets;
}


