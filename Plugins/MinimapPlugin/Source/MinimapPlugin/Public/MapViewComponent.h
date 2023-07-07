// Journeyman's Minimap by ZKShao.

#pragma once

#include "Components/BoxComponent.h"
#include "MapEnums.h"
#include "MapViewComponent.generated.h"

// MapViewComponent event signatures
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapViewCategoriesChangedSignature, UMapViewComponent*, MapView);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapViewSizeChangedSignature, UMapViewComponent*, MapView);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapViewDestroyedSignature, UMapViewComponent*, MapView);

class UMapIconComponent;
class AMapBackground;

// Represents a world area to render to a map, in terms of a location, rotation and XY view size.
// Add this to any character or other actor which serves as a center point for a map or minimap. 
// Note that for your convenience MapBackground actors have a MapViewComponent that you can use 
// to render exactly the area covered by the MapBackground actor.
UCLASS(ClassGroup=(MinimapPlugin), meta=(BlueprintSpawnableComponent))
class MINIMAPPLUGIN_API UMapViewComponent : public UBoxComponent
{
	GENERATED_BODY()

public:	
	UMapViewComponent();
	
#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif

	// Begin USceneComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End USceneComponent interface
	
	// Affects visibility of any MapIcon with this IconCategory
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIconCategoryVisible(FName IconCategory, const bool bNewVisible);
	// Returns visibility of MapIcons with this IconCategory
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsIconCategoryVisible(FName IconCategory) const;
	
	// Sets how far the map will display in world units
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetViewExtent(const float NewViewExtentX, const float NewViewExtentY);
	// Returns how far the map will display in world units
	UFUNCTION(BlueprintPure, Category = "Minimap")
	void GetViewExtent(float& ViewExtentX, float& ViewExtentY) const;

	// Set how far the minimap is zoomed out. The total world area visible is ZoomScale x ViewExtent
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetZoomScale(const float NewZoomScale);
	// Get how far the minimap is zoomed out. The total world area visible is ZoomScale x ViewExtent
	UFUNCTION(BlueprintPure, Category = "Minimap")
	float GetZoomScale() const;

	// Returns the aspect ratio Width / Height of the view
	UFUNCTION(BlueprintPure, Category = "Minimap")
	float GetViewAspectRatio() const;
	// Computes the world position of the view's rectangular frustum
	UFUNCTION(BlueprintPure, Category = "Minimap")
	TArray<FVector> GetWorldCorners();

	// Broad check for whether a component is possibly in view. False outcomes are always correct. True outcomes need to be further inspected.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool ViewContains(const FVector& WorldPos, const float WorldRadius) const;
	// Convert world position to view position, where the boundaries represented by view size correspond to 0.0 and 1.0
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	bool GetViewCoordinates(const FVector& WorldPos, bool bForceRectangular, float& U, float& V);
	// Convert world yaw to view yaw
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void GetViewYaw(const float WorldYaw, float& Yaw);
	// Converts normalized view position to world position
	UFUNCTION(BlueprintPure, Category = "Minimap")
	void DeprojectViewToWorld(const float U, const float V, FVector& WorldPos);
	
	// Retrieves the cached height level for a multi-level map background
	UFUNCTION(BlueprintPure, Category = "Minimap")
	int32 GetActiveBackgroundPriority(bool& IsInsideAnyBackground);
	// Retrieves the cached height level for a multi-level map background
	UFUNCTION(BlueprintPure, Category = "Minimap")
	int32 GetActiveBackgroundLevel(const AMapBackground* MapBackground);
	// Computes whether an icon is considered on the same level, to be rendered
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsSameBackgroundLevel(const UMapIconComponent* MapIcon);

private:
	UFUNCTION()
	void RegisterMultiLevelMapBackground(AMapBackground* MapBackground);
	UFUNCTION()
	void UnregisterMultiLevelMapBackground(AMapBackground* MapBackground);

	// Updates the box extent after any of its parameters have changed
	void UpdateViewSize();
	// Precomputed values to efficiently perform transformations to and from minimap view space.
	void UpdateTransformCache();
	// Recomputes the map view's height level on all tracked multi level backgrounds
	void UpdateBackgroundCache();
	
public:
	// Event that fires when visible icon categories change
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapViewCategoriesChangedSignature OnVisibleCategoriesChanged;
	// Event that fires when the view size is changed
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapViewSizeChangedSignature OnViewSizeChanged;
	// Event that fires when the view component is destroyed
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapViewDestroyedSignature OnViewDestroyed;

	// How the map view's runtime rotation will be updated. Set to InheritYaw for rotating minimaps and UseFixedRotation for non-rotating top down or side scrolling maps.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	EMapViewRotationMode RotationMode;
	// If RotationMode is set to UseFixedRotation, what world rotation the map view will use
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FRotator FixedRotation = FRotator::ZeroRotator;
	// If RotationMode is set to InheritYaw, the offset to add to the parent component's yaw to obtain this map view's yaw
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float InheritedYawOffset = 90.0f;
	// Uncheck to disable zooming with this view
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	bool bSupportZooming = true;
	// If set, this scene component's Z coordinate is used for selecting background levels
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	USceneComponent* HeightProxy;

	// The interval at which the map view recomputes his height level position on multi-level backgrounds.
	// When the map view is inside a multi-level background volume, icons on other levels aren't rendered.
	UPROPERTY(EditAnywhere, Category = "Minimap")
	float BackgoundLevelCacheLifetime = 0.05f;
	
private:
	// Precomputed values to efficiently perform transformations which potentially are done many times per rendered frame,
	// such as transforming world coordinates to minimap coordinates. These precomputed values are updated when a change in 
	// view transformation is detected.
	FTransform LastTransform;
	FTransform CachedTransform;
	FTransform CachedInverseTransform;
	FVector2D CachedInverseViewSize;
	float InverseViewRadius;
	
	// Timestamp of the last time the map view recomputed his position on background levels
	float LastBackgroundLevelComputeTime = 0.0f;

	// Priority level of surrounding background with highest priority
	int32 BackgroundPriority = INT_MIN;
	bool bInsideMultiLevelBackground = false;
	bool bInsideAnyBackground = false;
	
	// Per multi-level background, the height level this map view is on.
	UPROPERTY(Transient)
	TSet<AMapBackground*> MapBackgrounds;
	// Per multi-level background, the height level this map view is on.
	UPROPERTY(Transient)
	TMap<AMapBackground*, int32> PositionOnMultiLevelBackgrounds;

	TSet<FName> HiddenIconCategories;
	
};
