// Journeyman's Minimap by ZKShao.

#pragma once

#include "MapEnums.generated.h"

// Icon size can be defined in screen or world units
UENUM(BlueprintType)
enum class EIconSizeUnit : uint8
{
	// Icon size is defined in pixels. Will always appear as the same size, no matter the zoom level.
	ScreenSpace,
	// Icon size is defined in world units. When zooming in and out, the icon will scale up and down accordingly.
	WorldSpace,
};

// The way in which an actor reveals fog
UENUM(BlueprintType)
enum class EMapFogRevealMode : uint8
{
	// When reveal mode is Off, this actor won't reveal anything on the minimap
	Off,
	// When reveal mode is Temporary, this actor will reveal an area on the minimap while hs is nearby
	Temporary,
	// When reveal mode is Permanent, areas this actor visits will be permanently revealed. However, icons may still be configured to be hidden unless a revealing actor is currently nearby.
	Permanent,
};

// Icon size can be defined in screen or world units
UENUM(BlueprintType)
enum class EIconFogInteraction : uint8
{
	// Icon is only visible when the icon location is being revealed currently. Reveal factor must exceed FogRevealThreshold.
	OnlyRenderWhenRevealing,
	// Icon is only visible when the icon location is revealed or has been revealed. Reveal factor must exceed FogRevealThreshold.
	OnlyRenderWhenExplored,
	// Icon is always visible but under the fog. Opaque fog will hide the icon.
	AlwaysRenderUnderFog,
	// Icon is always visible on top of fog
	AlwaysRenderAboveFog,
};

// Icon size can be defined in screen or world units
UENUM(BlueprintType)
enum class EIconBackgroundInteraction : uint8
{
	// Icon visibility is not affected by MapBackgrounds
	AlwaysRender,
	// Icon is only visible when inside the same MapBackground as the MapViewComponent
	OnlyRenderInSameVolume,
	// Icon is only visible when inside the same MapBackground as the MapViewComponent, and on the same floor
	OnlyRenderOnSameFloor,
	// Same as OnlyRenderInSameVolume, but the MapViewComponent must be in an equally high priority MapBackground as this icon
	OnlyRenderInPriorityVolume,
	// Same as OnlyRenderOnSameFloor, but the MapViewComponent must be in an equally high priority MapBackground as this icon
	OnlyRenderOnPriorityFloor,
};

// Utility option for minimap renderers to auto-locate a MapViewComponent in the world
UENUM(BlueprintType)
enum class EMapViewSearchOption : uint8
{
	// The minimap will try to find a MapViewComponent anywhere in the world, prioritizing in this order: players, background actors, fog actors, other actors
	Any,
	// The minimap will try to find a MapViewComponent on the first player's pawn or controller
	OnPlayer,
	// The minimap will try to find a MapViewComponent on a background actor
	OnMapBackground,
	// The minimap will try to find a MapViewComponent on a fog actor
	OnMapFog,
	// The minimap will not try to find a MapViewComponent. You must manually supply it.
	Disabled,
};

// Utility option for map fog to auto-locate a PostProcessVolume to add the fog post process effect to
UENUM(BlueprintType)
enum class EFogPostProcessVolumeOption : uint8
{
	// An unbound post process volume will be looked for in the level
	AutoLocate,
	// An unbound post process volume will be looked for in the level. If not found, it will be created.
	AutoLocateOrCreate,
	// No post process volume will be looked for. You must manually set it, or no fog post process effect is applied.
	Manual,
};

// Icon size can be defined in screen or world units
UENUM(BlueprintType)
enum class EMapViewRotationMode : uint8
{
	// The map view always uses FixedRotation as world rotation
	UseFixedRotation,
	// The map view inherits the parent component's yaw and adds InheritedYawOffset
	InheritYaw,
};