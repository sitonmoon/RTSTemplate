// Journeyman's Minimap by ZKShao.

#pragma once

#include "Components/ActorComponent.h"
#include "Types/SlateEnums.h"
#include "Layout/Margin.h"
#include "MapEnums.h"
#include "MapRendererComponent.generated.h"

class UMapTrackerComponent;
class UMapViewComponent;
class UMapIconComponent;
class UCanvas;
class UMaterialInterface;
class UMaterialInstanceDynamic;

// MapRendererComponent event signatures
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMapClickedSignature, FVector, WorldLocation, bool, bIsLeftMouseButton);

// Given a MapViewComponent, renders a map of the area represented by the map view to a HUD Canvas.
// Add this component to your game's HUD class in case you want to render a map using the Canvas approach.
// Alternatively, ignore this component and use the UMG approach by adding a 'Minimap' widget to the game viewport.
UCLASS(ClassGroup=(MinimapPlugin), meta=(BlueprintSpawnableComponent))
class MINIMAPPLUGIN_API UMapRendererComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMapRendererComponent();

	// Begin UActorComponent interface
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// End UActorComponent interface

	// Affects whether a MapView is automatically located at game start. Can be called during gameplay to find a new MapView, replacing the current.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetAutoLocateMapView(const EMapViewSearchOption InAutoLocateMapView);

	// Should be called from within your HUD's DrawHUD
	void DrawToCanvas(UCanvas* Canvas);

	// HUD clicks that are potentially map click events should be passed to this function. Will fire click events on any 
	// icons being mouse-overed. Otherwise will fire a background click event if the cursor is on the map. 
	// Returns true if the map consumed the click event.
	bool HandleClick(const FVector2D& ScreenPosition, const bool bIsLeftClick);

	// Sets the view component which defines the location, rotation and view distance of the rendered map
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetMapView(UMapViewComponent* InMapView);

	// Set whether the rendered map is circular
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIsCircular(const bool bNewIsCircular);
	// Returns whether the rendered map is circular
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsCircular() const;
	// Set whether the map is currently rendered
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIsRendered(const bool bNewIsRendered);
	// Returns whether the map is currently rendered
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsRendered() const;

	// Set whether the boundaries of the player's view is visualized on the map as a trapezoid
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetDrawFrustum(const bool bNewDrawFrustum);
	// Returns whether the boundaries of the player's view is visualized on the map as a trapezoid
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool GetDrawFrustum() const;
	
	// If bDrawFrustum == true, how far the floor is beneath the MapView. Controls the size of the rendered trapezoid.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetFrustumFloorDistance(const float NewFrustumFloorDistance);
	// If bDrawFrustum == true, how far the floor is beneath the MapView. Controls the size of the rendered trapezoid.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	float GetFrustumFloorDistance() const;
	
	// Sets the color to show underneath any background textures
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetBackgroundFillColor(const FLinearColor& NewBackgroundFillColor);
	// Retrieves the color that is shown underneath any background textures
	UFUNCTION(BlueprintPure, Category = "Minimap")
	FLinearColor GetBackgroundFillColor() const;

	// Set how the map should align horizontally in the viewport
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment);
	// Set how the map should align vertically in the viewport
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetVerticalAlignment(EVerticalAlignment InVerticalAlignment);
	// Set how far from the viewport's edge the map should be rendered. The alignment settings affect which values are used.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetMargin(const int32 Left, const int32 Top, const int32 Right, const int32 Bottom);
	// Sets the rendered size of the map. The alignment settings affect which values are used.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetSize(const int32 Width, const int32 Height);

private:
	void AutoRelocateMapView();

	// Fires any buffered hover events that were detected in the rendering thread, exploiting essential rendering computations
	void TickHoverEvents();
	// Clears any on-going hover events, for example when the map rendering is stopped
	void ClearHoverEvents();
	// Starts a hovered event for the icon
	void MarkOnHoverStart(UMapIconComponent* MapIcon);
	// Ends a hovered event for the icon
	void MarkOnHoverEnd(UMapIconComponent* MapIcon);

	// Computes which region within the viewport to utilize given current alignment, margin and size settings
	void ComputeCanvasRect(UCanvas* Canvas, FVector2D& MapTopLeft, FVector2D& MapSize);
	// Computes which subregion to render the map to, also taking into account the view component's aspect ratio
	void ComputeRenderRegion(const FVector2D& MapTopLeft, const FVector2D& MapSize, FVector2D& RenderRegionTopLeft, FVector2D& RenderRegionSize);
	// Draws the map to the canvas
	void RenderToCanvas(UCanvas* Canvas, const FVector2D& MapTopLeft, const FVector2D& MapSize);
	
	void DrawBackground(UCanvas* Canvas, const FVector2D& RenderRegionTopLeft, const FVector2D& RenderRegionSize);
	void DrawFog(UCanvas* Canvas, const FVector2D& RenderRegionTopLeft, const FVector2D& RenderRegionSize);
	void DrawIcons(UCanvas* Canvas, const FVector2D& RenderRegionTopLeft, const FVector2D& RenderRegionSize, const bool bAboveFog);
	void DrawBoundary(UCanvas* Canvas, const FVector2D& RenderRegionTopLeft, const FVector2D& RenderRegionSize);
	void DrawFrustum(UCanvas* Canvas, const FVector2D& RenderRegionTopLeft, const FVector2D& RenderRegionSize);

public:
	// Event that fires when the background is clicked. When an icon is clicked, this event is not fired.
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapClickedSignature OnMapClicked;

protected:
	// Whether a MapViewComponent should be found automatically in the world at game start. If disabled, call SetMapView() manually with a valid MapView.
	UPROPERTY(EditAnywhere, Category = "Minimap")
	EMapViewSearchOption AutoLocateMapView = EMapViewSearchOption::Any;
	// Whether the rendered map is circular
	UPROPERTY(EditAnywhere, Category = "Minimap")
	bool bIsCircular = false;
	// Whether the map is currently being rendered
	UPROPERTY(EditAnywhere, Category = "Minimap")
	bool bIsRendered = true;
	// Whether the player's frustum is visualized as a trapezoid. This is done by intersecting 
	UPROPERTY(EditAnywhere, Category = "Minimap")
	bool bDrawFrustum = false;
	// Affects the drawn frustum's size when bDrawFrustum is true. Distance between player camera and the floor.
	UPROPERTY(EditAnywhere, Category = "Minimap")
	float FrustumFloorDistance = 300.0f;
	// The color shown in places with no assigned background texture or where the texture is transparent.
	UPROPERTY(EditAnywhere, Category = "Minimap")
	FLinearColor BackgroundFillColor = FLinearColor(0, 0, 0, 1);
	// The map's horizontal alignment in the viewport
	UPROPERTY(EditAnywhere, Category = "Minimap")
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;
	// The map's vertical alignment in the viewport
	UPROPERTY(EditAnywhere, Category = "Minimap")
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;
	// How far from the viewport's edge the map should be rendered. The alignment settings affect which values are used.
	UPROPERTY(EditAnywhere, Category = "Minimap")
	FMargin Margin = FMargin(0, 0, 0, 0);
	// The rendered size of the map. The alignment settings affect which values are used.
	UPROPERTY(EditAnywhere, Category = "Minimap")
	FVector2D Size = FVector2D(200, 200);
	// The material used to fill the background of the material for regions where no background texture is rendered
	UPROPERTY(EditAnywhere, Category = "Minimap")
	UMaterialInterface* FillMaterial;
	
private:
	// Material instance of the background fill material
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* FillMaterialInstance;
	// Map tracker which will be found in the world at begin play
	UPROPERTY(Transient)
	UMapTrackerComponent* MapTracker;
	// Map view which defines what part of the world is rendered
	UPROPERTY(Transient)
	UMapViewComponent* MapView;

	// Icons that are currently being hovered
	UPROPERTY(Transient)
	TSet<UMapIconComponent*> HoveringIcons;
	// Icons that will fire their hover start event during the next tick. Detected during rendering pass to leverage computations.
	UPROPERTY(Transient)
	TArray<UMapIconComponent*> BufferedHoverStartEvents;
	// Icons that will fire their hover start end during the next tick. Detected during rendering pass to leverage computations.
	UPROPERTY(Transient)
	TArray<UMapIconComponent*> BufferedHoverEndEvents;
	// The most recent canvas that was rendered to. Used to transform screen space mouse events to world space.
	UPROPERTY(Transient)
	UCanvas* LastCanvas;
	
};
