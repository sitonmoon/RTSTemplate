// Journeyman's Minimap by ZKShao.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Layout/Margin.h"
#include "MapEnums.h"
#include "MapFunctionLibrary.generated.h"

class UCanvas;
class UMapTrackerComponent;
class UMapViewComponent;
class AMapBackground;

// Utility functions for the minimap plugin
UCLASS()
class MINIMAPPLUGIN_API UMapFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	// Retrieves the central MapTrackerComponent component.
	UFUNCTION(BlueprintPure, Category = "Minimap", meta=(WorldContext="WorldContextObject"))
	static UMapTrackerComponent* GetMapTracker(const UObject* WorldContextObject);
	// Retrieves the first MapBackground placed in the level, if any. If you expect more than one MapBackground, you shouldn't use this function.
	UFUNCTION(BlueprintPure, Category = "Minimap", meta=(WorldContext="WorldContextObject"))
	static AMapBackground* GetFirstMapBackground(const UObject* WorldContextObject);
	// Utility option to find a MapViewComponent in the world
	UFUNCTION(BlueprintCallable, Category = "Minimap", meta = (WorldContext = "WorldContextObject"))
	static UMapViewComponent* FindMapView(UObject* WorldContextObject, const EMapViewSearchOption MapViewSearchOption);
	
	// Computes whether an icon is visible in view, given its UV coordinates and UV size in normalized view space.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	static bool DetectIsInView(const FVector2D& UV, const FVector2D& OuterRadiusUV, const bool bIsCircular);
	// Moves an element completely into minimap space, given its UV coordinates and UV size.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	static FVector2D ClampIntoView(const FVector2D& UV, const float OuterRadiusUV, const bool bIsCircular);
	// Given a box selection in a view, gathers all map icons that intersect with the box.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	static TArray<UMapIconComponent*> BoxSelectInView(const FVector2D& StartUV, const FVector2D& EndUV, UMapViewComponent* MapView, const bool bIsCircular);

	// Computes a top-down trapezoid that represents the player camera's view frustum. Trapezoid is computed by generating 4 lines through the corners of the player's viewport and 
	// computing their intersection with the up-facing floor at 'FloorDistance' distance away from the camera. Returns true only when the view frustum could be computed. It fails
	// when the player is looking upward or if looking too closely towards the horizon. On success, CornerUVs stores the 4 points in minimap space that form the trapezoid.
	UFUNCTION(BlueprintCallable, Category = "Minimap", meta = (WorldContext = "WorldContextObject"))
	static bool ComputeViewFrustum(const UObject* WorldContextObject, UMapViewComponent* MapView, const bool bIsCircular, TArray<FVector2D>& CornerUVs, const float FloorDistance = 600.0f);

};
