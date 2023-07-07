// Journeyman's Minimap by ZKShao.

#include "MapTrackerComponent.h"
#include "MinimapPluginPrivatePCH.h"
#include "MapFog.h"
#include "EngineUtils.h"

UMapTrackerComponent::UMapTrackerComponent()
{
}

void UMapTrackerComponent::RegisterMapIcon(UMapIconComponent* MapIcon)
{
	MapIcons.Add(MapIcon);
	OnMapIconRegistered.Broadcast(MapIcon);
}

void UMapTrackerComponent::UnregisterMapIcon(UMapIconComponent* MapIcon)
{
	MapIcons.RemoveSingle(MapIcon);
	OnMapIconUnregistered.Broadcast(MapIcon);
}

const TArray<UMapIconComponent*>& UMapTrackerComponent::GetMapIcons() const
{
	return MapIcons;
}

void UMapTrackerComponent::RegisterMapBackground(AMapBackground* MapBackground)
{
	MapBackgrounds.Add(MapBackground);
	OnMapBackgroundRegistered.Broadcast(MapBackground);
}

void UMapTrackerComponent::UnregisterMapBackground(AMapBackground* MapBackground)
{
	MapBackgrounds.RemoveSingle(MapBackground);
	OnMapBackgroundUnregistered.Broadcast(MapBackground);
}

const TArray<AMapBackground*>& UMapTrackerComponent::GetMapBackgrounds() const
{
	return MapBackgrounds;
}

void UMapTrackerComponent::RegisterMapFog(AMapFog* MapFog)
{
	MapFogs.Add(MapFog);
	OnMapFogRegistered.Broadcast(MapFog);
}

void UMapTrackerComponent::UnregisterMapFog(AMapFog* MapFog)
{
	MapFogs.RemoveSingle(MapFog);
	OnMapFogUnregistered.Broadcast(MapFog);
}

const TArray<AMapFog*>& UMapTrackerComponent::GetMapFogs() const
{
	return MapFogs;
}

bool UMapTrackerComponent::HasMapFog() const
{
	return MapFogs.Num() > 0;
}

float UMapTrackerComponent::GetFogRevealedFactor(const FVector& WorldLocation, const bool bRequireCurrentlyRevealing, bool& bIsInsideFogVolume) const
{
	float RevealFactor = 1.0f;
	bIsInsideFogVolume = false;
	for (AMapFog* MapFog : MapFogs)
	{
		if (MapFog->GetFogAtLocation(WorldLocation, bRequireCurrentlyRevealing, RevealFactor))
		{
			bIsInsideFogVolume = true;
			break;
		}
	}
	return RevealFactor;
}

void UMapTrackerComponent::RegisterMapRevealer(UMapRevealerComponent* MapRevealer)
{
	MapRevealers.Add(MapRevealer);
	OnMapRevealerRegistered.Broadcast(MapRevealer);
}

void UMapTrackerComponent::UnregisterMapRevealer(UMapRevealerComponent* MapRevealer)
{
	MapRevealers.RemoveSingle(MapRevealer);
	OnMapRevealerUnregistered.Broadcast(MapRevealer);
}

const TArray<UMapRevealerComponent*>& UMapTrackerComponent::GetMapRevealers() const
{
	return MapRevealers;
}