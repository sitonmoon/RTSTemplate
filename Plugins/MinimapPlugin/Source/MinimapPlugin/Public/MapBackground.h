// Journeyman's Minimap by ZKShao.

#pragma once

#include "MapAreaBase.h"
#include "MapBackground.generated.h"

class UBoxComponent;
class USceneCaptureComponent2D;
class UNavMeshRenderingComponent;
class UTextureRenderTarget2D;
class UMapViewComponent;
class UMapRendererComponent;

// MapBackground event signatures
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapBackgroundTextureChangedSignature, AMapBackground*, MapBackground);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapBackgroundMaterialChangedSignature, AMapBackground*, MapBackground);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapBackgroundAppearanceChangedSignature, AMapBackground*, MapBackground);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMapBackgroundRenderedSignature, AMapBackground*, MapBackground, int32, Level, UTextureRenderTarget2D*, RenderTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMapBackgroundOverlayChangedSignature, AMapBackground*, MapBackground, int32, Level, UTextureRenderTarget2D*, RenderTarget);

USTRUCT(BlueprintType)
struct FMapBackgroundLevel
{
	GENERATED_USTRUCT_BODY()

	// If set, this texture will be rendered as background. Otherwise, a generated background will be rendered.
	UPROPERTY(EditAnywhere, Category = "Minimap Snapshot Generation")
	UTexture2D* BackgroundTexture = nullptr;
	// Generated backgrounds are rendered to this render target. If not set to a static render target, a dynamic one will be created on demand.
	UPROPERTY(EditAnywhere, Category = "Minimap Snapshot Generation")
	UTextureRenderTarget2D* RenderTarget = nullptr;
	// An optional render target that you can draw custom icons and environment on. Just like the background texture, it will be mapped to the background volume. 
	// Tip: Use this MapBackground's MapView->GetViewCoordinates / GetViewYaw to convert world position and rotation to relative texture coordinates and render angle.
	UPROPERTY(EditAnywhere, Category = "Minimap Snapshot Generation")
	UTextureRenderTarget2D* Overlay = nullptr;
	// Level's world height measured from the top of the previous level. Value is ignored for the last level: it automatically fills the remaining space.
	UPROPERTY(EditAnywhere, Category = "Minimap Snapshot Generation")
	float LevelHeight = 0.0f;
	
	// Size of the sampled region from the background texture
	UPROPERTY(Transient)
	FVector2D SamplingResolution;

};

// If you want to use a background image in your minimap, place MapBackgrounds in your level.
// You can place any number of MapBackgrounds. MapBackgrounds may be spawned and destroyed during gameplay.
//
// The MapBackground has a box component. Move it and resize it in any way so that it covers the 
// the part of your level that you want to import a background texture for. 
//
// Whenever you move the actor, a top-down snapshot will be generated of the area, stored in 'StaticRenderTarget'.
// You can use this feature to get an image to draw over in an external image editor. Once you have prepared your
// own background texture, set it in 'BackgroundTexture'.
//
// You can also just use the generated snapshot in-game. In that case, leave 'BackgroundTexture' empty. If you want
// multiple MapBackgrounds to use a generated snapshot, clear 'StaticRenderTarget'. Then, each MapBackground will
// create its own dynamic render target and they won't write to the same render target anymore.
UCLASS()
class MINIMAPPLUGIN_API AMapBackground : public AMapAreaBase
{
	GENERATED_BODY()
	
public:	
	AMapBackground();

	// Begin AActor interface
#if WITH_EDITOR
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End AActor interface
	
	// Changes what material is used to render this volume's background texture in UMG
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetBackgroundMaterialForUMG(UMaterialInterface* NewMaterial);
	// Retrieves what material is used to render this volume's background texture in UMG
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UMaterialInterface* GetBackgroundMaterialForUMG() const;
	
	// Changes what material is used to render this volume's background texture on UCanvas
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetBackgroundMaterialForCanvas(UMaterialInterface* NewMaterial);
	// Retrieves what material is used to render this volume's background texture on UCanvas
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UMaterialInstanceDynamic* GetBackgroundMaterialInstanceForCanvas(UMapRendererComponent* Renderer);
	
	// Sets whether the background is visible on the minimap
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetBackgroundVisible(const bool bNewVisible);
	// Retrieves whether the background is visible on the minimap
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsBackgroundVisible() const;
	
	// Sets the background's priority. If the MapViewComponent is inside multiple MapBackgrounds at a time, only the ones with highest priority are rendered.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetBackgroundPriority(const int32 NewBackgroundPriority);
	// Retrieves the background's priority. If the MapViewComponent is inside multiple MapBackgrounds at a time, only the ones with highest priority are rendered.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	int32 GetBackgroundPriority() const;
	
	// Sets the background's ZOrder. If there are multiple MapBackgrounds in the level, the one with highest ZOrder is rendered on top in the minimap.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetBackgroundZOrder(const int32 NewBackgroundZOrder);
	// Retrieves the background's ZOrder. If there are multiple MapBackgrounds in the level, the one with highest ZOrder is rendered on top in the minimap.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	int32 GetBackgroundZOrder() const;
	
	// Returns whether this MapBackground has multiple height levels
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsMultiLevel() const;
	
	// Sets the background texture to render. If null, will generate a snapshot.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetBackgroundTexture(const int32 Level = 0, UTexture2D* NewBackgroundTexture = nullptr);
	// Returns whichever background texture is active
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UTexture* GetBackgroundTexture(const int32 Level = 0) const;
	
	// Sets the background texture to render. If null, will generate a snapshot.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetBackgroundOverlay(const int32 Level = 0, UTextureRenderTarget2D* NewBackgroundOverlay = nullptr);
	// Returns whichever background texture is active
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UTextureRenderTarget2D* GetBackgroundOverlay(const int32 Level = 0) const;

	// Returns whichever background texture is active
	virtual int32 GetLevelAtHeight(const float WorldZ) const override;

	// Returns whichever background texture is active
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UTexture* GetBackgroundTextureAtHeight(const float WorldZ) const;
	
	// Can be called to re-render the map background from the top down camera. Only has effect when the BackgroundTexture is null, thus a rendered background is used.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void RerenderBackground();
	
protected:
	// If the background's aspect ratio isn't 1:1, the square render target will have irrelevant parts. This transformation maps UVs to only the relevant part of the texture.
	virtual FVector2D CorrectUVs(const int32 Level, const FVector2D& InUV) const;

private:
#if WITH_EDITOR
	void VisualizeLevelsInEditor();
#endif

	void NormalizeScale();

	// 
	void InitializeDynamicRenderTargets();

	// Decides whether to take a top down snapshot
	void ApplyBackgroundTexture();
	// Generates the top down snapshot
	void GenerateSnapshot(UTextureRenderTarget2D* RenderTarget, float RelativeHeight);

public:
	// Event that fires when the background texture changes
	UPROPERTY(BlueprintAssignable, Category = "Minimap Background")
	FMapBackgroundTextureChangedSignature OnMapBackgroundTextureChanged;
	// Event that fires when the material used to render the background changes
	UPROPERTY(BlueprintAssignable, Category = "Minimap Background")
	FMapBackgroundMaterialChangedSignature OnMapBackgroundMaterialChanged;
	// Event that fires when any aspect of the background's appearance changes
	UPROPERTY(BlueprintAssignable, Category = "Minimap Background")
	FMapBackgroundAppearanceChangedSignature OnMapBackgroundAppearanceChanged;
	// Event that fires whenever a top down render is captured. For background levels without a static texture, this happens at BeginPlay and when calling RerenderBackground.
	UPROPERTY(BlueprintAssignable, Category = "Minimap Background")
	FMapBackgroundRenderedSignature OnMapBackgroundRendered;
	// Event that fires when the overlay render target is changed.
	UPROPERTY(BlueprintAssignable, Category = "Minimap Background")
	FMapBackgroundOverlayChangedSignature OnMapBackgroundOverlayChanged;

protected:
	// Assign background textures to height levels within the volume
	UPROPERTY(EditAnywhere, Category = "Minimap Background")
	TArray<FMapBackgroundLevel> BackgroundLevels;
	// Material is used to render the background in UMG. It receives the relevant background texture as a texture input named 'Texture'.
	UPROPERTY(EditAnywhere, Category = "Minimap Background")
	UMaterialInterface* BackgroundMaterial_UMG;
	// Material is used to render the background to Canvas. It receives the relevant background texture as a texture input named 'Texture'.
	UPROPERTY(EditAnywhere, Category = "Minimap Background")
	UMaterialInterface* BackgroundMaterial_Canvas;
	
	// Whether the background is currently rendered
	UPROPERTY(EditAnywhere, Category = "Minimap Background")
	bool bBackgroundVisible = true;
	// When the map view is inside multiple backgrounds, only the background(s) with highest priority are rendered
	UPROPERTY(EditAnywhere, Category = "Minimap Background")
	int32 BackgroundPriority;
	// When multiple backgrounds are rendered, backgrounds with higher ZOrder are rendered on top
	UPROPERTY(EditAnywhere, Category = "Minimap Background")
	int32 BackgroundZOrder;
	
	// When creating dynamic render targets, the width and height of the render target. Increase to capture more detail in the snapshot at the cost of more texture memory usage.
	UPROPERTY(EditAnywhere, Category = "Minimap Background Generation")
	int32 DynamicRenderTargetSize = 1024;
	// If true, navigation mesh will be included in the generated snapshot
	UPROPERTY(EditAnywhere, Category = "Minimap Background Generation")
	bool bRenderNavigationMesh = true;
	// Actors of these classes are hidden from the generated background
	UPROPERTY(EditAnywhere, Category = "Minimap Background Generation")
	TArray<TSubclassOf<AActor>> HiddenActorClasses;
	// These actors in the level are hidden from the generated backgrounds
	UPROPERTY(EditAnywhere, Category = "Minimap Background Generation")
	TArray<AActor*> HiddenActors;

private:
	// Instance of the background material per MapRendererComponent. Only used when rendering to a UCanvas.
	UPROPERTY(Transient)
	TMap<UMapRendererComponent*, UMaterialInstanceDynamic*> MaterialInstances;
	// Non-interactable box components used purely for visualizing the floor configuration in wireframe in editor viewports
	UPROPERTY(Transient)
	TArray<UBoxComponent*> LevelVisualizers;

	// The time at which the material was last changed, used to update the material instance's Time parameter
	float AnimStartTime;

	// Used to generate a background if no hand drawn background texture is set
	UPROPERTY(VisibleAnywhere, Category = "Minimap Background Generation")
	USceneCaptureComponent2D* CaptureComponent2D;
	// Used to include the navigation mesh in the generated background
	UPROPERTY(VisibleAnywhere, Category = "Minimap Background Generation")
	UNavMeshRenderingComponent* NavMeshRenderingComponent;
	
};
