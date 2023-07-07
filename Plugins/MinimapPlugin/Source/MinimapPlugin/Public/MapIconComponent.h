// Journeyman's Minimap by ZKShao.

#pragma once

#include "Components/BillboardComponent.h"
#include "MapEnums.h"
#include "MapIconComponent.generated.h"

class UMapTrackerComponent;
class UMapViewComponent;
class UMapRendererComponent;

// MapIconComponent event signatures
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapIconMaterialChangedSignature, UMapIconComponent*, MapIcon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapIconMaterialInstancesChangedSignature, UMapIconComponent*, MapIcon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapIconAppearanceChangedSignature, UMapIconComponent*, MapIcon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMapIconEnteredViewSignature, UMapIconComponent*, MapIcon, UMapViewComponent*, View);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMapIconLeftViewSignature, UMapIconComponent*, MapIcon, UMapViewComponent*, View);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapIconDestroyedSignature, UMapIconComponent*, MapIcon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapIconHoverStartSignature, UMapIconComponent*, MapIcon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapIconHoverEndSignature, UMapIconComponent*, MapIcon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMapIconClickedSignature, UMapIconComponent*, MapIcon, bool, bIsLeftMouse);

// A MapIconComponent represents an icon to render on minimaps. 
// To make an actor appear on a minimap, add this component to it and then configure it. Icon properties can be 
// set in C++ and in blueprint. Properties can be changed during gameplay and any changes will fire events so 
// that any existing icon instances will update their appearance right away.
UCLASS(ClassGroup=(MinimapPlugin), hidecategories=(Sprite), meta=(BlueprintSpawnableComponent))
class MINIMAPPLUGIN_API UMapIconComponent : public UBillboardComponent
{
	GENERATED_BODY()

public:	
	UMapIconComponent();

	// Begin UActorComponent interface
#if WITH_EDITOR
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent) override;
#endif

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End UActorComponent interface
	
	// Sets the material used to render the icon in UMG
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIconMaterialForUMG(UMaterialInterface* NewMaterial);
	// Retrieves the material used to render the icon in UMG
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UMaterialInterface* GetIconMaterialForUMG() const;
	// Retrieves the material used to render the objective arrow in UMG
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UMaterialInterface* GetObjectiveArrowMaterialForUMG() const;
	// Resets the material used to render the icon in UMG to its initial material
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void ResetIconMaterialForUMG();
	// Gets all current UMG icon material instances for this icon
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void GetIconMaterialInstancesForUMG(TArray<UMaterialInstanceDynamic*>& MaterialInstances);

	// Internal use only: UMG icons register their material instances
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void RegisterMaterialInstanceFromUMG(UUserWidget* IconWidget, UMaterialInstanceDynamic* MatInst);

	// Sets the material used to render the icon on UCanvas
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIconMaterialForCanvas(UMaterialInterface* NewMaterial);
	// Retrieves the material used to render the icon on UCanvas
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UMaterialInterface* GetIconMaterialForCanvas() const;
	// Retrieves the material used to render the objective arrow on UCanvas
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UMaterialInterface* GetObjectiveArrowMaterialForCanvas() const;
	// Resets the material used to render the icon on UCanvas to its initial material
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void ResetIconMaterialForCanvas();
	// Gets all current canvas icon material instances for this icon
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void GetIconMaterialInstancesForCanvas(TArray<UMaterialInstanceDynamic*>& MaterialInstances);

	// Sets the icon's texture
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIconTexture(UTexture2D* NewIcon);
	// Retrieves the icon's texture
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UTexture2D* GetIconTexture() const;
	
	// Sets the icon's name to be displayed in mouse-over tooltips
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIconTooltipText(FName NewIconName);
	// Retrieves the icon's name to be displayed in mouse-over tooltips
	UFUNCTION(BlueprintPure, Category = "Minimap")
	FName GetIconTooltipText() const;

	// Sets the icon's visibility on minimap
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIconVisible(const bool bNewVisible);
	// Retrieves whether the icon is visible on minimap. This is simply its visibility setting and not related from whether it is currently in view.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsIconVisible() const;
	
	// Sets whether the icon is interactable. If the icon is not interactable, it will not respond to mouse-over or click events.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIconInteractable(const bool bNewInteractable);
	// Retrieves whether the icon is interactable. If the icon is not interactable, it will not respond to mouse-over or click events.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsIconInteractable() const;

	// Sets whether the icon will rotate to represent its actor's rotation
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIconRotates(const bool bNewRotates);
	// Retrieves whether the icon should rotate to represent its actor's rotation
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool DoesIconRotate() const;

	// Sets the icon's render size in pixels prior to DPI scaling
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIconSize(const float NewIconSize, const EIconSizeUnit NewIconSizeUnit);
	// Retrieves the icon's render size in pixels prior to DPI scaling
	UFUNCTION(BlueprintPure, Category = "Minimap")
	float GetIconSize() const;
	// Retrieves whether the icon size is defined in pixels (prior to applying DPI scaling) or world units.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	EIconSizeUnit GetIconSizeUnit() const;
	
	// Sets the icon's draw color, which is multiplied by the texture sample to obtain the final color
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIconDrawColor(const FLinearColor& NewDrawColor);
	// Retrieves the icon's draw color, which is multiplied by the texture sample to obtain the final color
	UFUNCTION(BlueprintPure, Category = "Minimap")
	FLinearColor GetIconDrawColor() const;
	
	// Sets the icon's Z order. Icons with higher Z-Order are rendered on top.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIconZOrder(const int32 NewZOrder);
	// Retrieves the icon's Z order. Icons with higher Z-Order are rendered on top.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	int32 GetIconZOrder() const;
	
	// Sets whether the icon will stay at the minimap's edge when its actor is falls outside the minimap's view range
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetObjectiveArrowEnabled(const bool bNewObjectiveArrowEnabled);
	// Retrieves whether the icon should stay at the minimap's edge when its actor is falls outside the minimap's view range
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsObjectiveArrowEnabled() const;

	// Sets what texture to use when showing at the minimap's edge. If none is set, will use the default icon
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetObjectiveArrowTexture(UTexture2D* NewTexture);
	// Retrieves what texture to use when showing at the minimap's edge
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UTexture2D* GetObjectiveArrowTexture() const;
	
	// Sets whether the icon will point to its actor when shown at the minimap's edge
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetObjectiveArrowRotates(const bool bNewRotates);
	// Retrieves whether the icon should point to its actor when shown at the minimap's edge
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool DoesObjectiveArrowRotate() const;
	
	// Sets the icon's render size in pixels prior to DPI scaling
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetObjectiveArrowSize(const float NewObjectiveArrowSize);
	// Retrieves the icon's render size in pixels prior to DPI scaling
	UFUNCTION(BlueprintPure, Category = "Minimap")
	float GetObjectiveArrowSize() const;
	
	// Sets how the icon's visibility reacts to multi-level backgrounds.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIconBackgroundInteraction(const EIconBackgroundInteraction NewBackgroundInteraction);
	// Retrieves how the icon's visibility reacts to multi-level backgrounds.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	EIconBackgroundInteraction GetIconBackgroundInteraction() const;
	
	// Sets how the icon's visibility reacts to fog.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIconFogInteraction(const EIconFogInteraction NewFogInteraction);
	// Retrieves how the icon's visibility reacts to fog.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	EIconFogInteraction GetIconFogInteraction() const;
	
	// Sets the required fog reveal factor to make the icon appear
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIconFogRevealThreshold(const float NewFogRevealThreshold);
	// Retrieves the required fog reveal factor to make the icon appear
	UFUNCTION(BlueprintPure, Category = "Minimap")
	float GetIconFogRevealThreshold() const;
	
	// Retrieves material instance to render the icon with on HUD Canvas.
	UMaterialInstanceDynamic* GetIconMaterialInstanceForCanvas(UMapRendererComponent* Renderer);
	// Retrieves material instance to render objective arrow with on HUD Canvas.
	UMaterialInstanceDynamic* GetObjectiveArrowMaterialInstanceForCanvas(UMapRendererComponent* Renderer);

	// Mark the icon as rendered or not rendered in a specific view, firing an OnIconEnteredView or OnIconLeftView event
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	bool MarkRenderedInView(UMapViewComponent* View, const bool bNewIsRendered);
	// Retrieves whether the icon is currently visible in a specific view
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsRenderedInView(UMapViewComponent* View) const;
	
	// Mark the icon as currently being mouse-overed, firing an OnIconHoverStart event
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void ReceiveHoverStart();
	// Mark the icon as no longer being mouse-overed, firing an OnIconHoverEnd event
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void ReceiveHoverEnd();
	// Notify icon that it is clicked, firing an OnIconClicked event
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void ReceiveClicked(const bool bIsLeftMouseButton);

private:
	// Updates the preview sprite to show in the editor viewport
	void RefreshPreviewSprite();

	// Mark the icon not rendered from all views, firing OnViewLeft events. Called, for example, prior to removing the icon from the world.
	void UnmarkRenderedFromAllViews();

public:
	// Event that fires whenever the icon's appearance changes
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapIconAppearanceChangedSignature OnIconAppearanceChanged;
	// Event that fires whenever the icon's material changes
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapIconMaterialChangedSignature OnIconMaterialChanged;
	// Event that fires when the set of material instances changes (i.e. new minimap was created or material changed)
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapIconMaterialInstancesChangedSignature OnIconMaterialInstancesChanged;
	// Event that fires whenever the icon comes into a specific MapViewComponent's view
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapIconEnteredViewSignature OnIconEnteredView;
	// Event that fires whenever the icon leaves a specific MapViewComponent's view
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapIconLeftViewSignature OnIconLeftView;
	// Event that fires whenever the icon is destroyed
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapIconDestroyedSignature OnIconDestroyed;
	// Event that fires whenever the icon is mouse-overed
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapIconHoverStartSignature OnIconHoverStart;
	// Event that fires whenever the icon is no longer mouse-overed
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapIconHoverEndSignature OnIconHoverEnd;
	// Event that fires whenever the icon is clicked
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapIconClickedSignature OnIconClicked;

protected:
	// Icons can be hidden by category on a specific minimap via MapView->SetIconCategoryVisible()
	UPROPERTY(EditAnywhere, Category = "Minimap Icon Rendering", BlueprintReadOnly)
	FName IconCategory = NAME_None;
	// The texture to render for this actor. If none is set, will render a rectangle.
	UPROPERTY(EditAnywhere, Category = "Minimap Icon Rendering")
	UTexture2D* IconTexture;
	// Material to render the icon with to UMG minimaps
	UPROPERTY(EditAnywhere, Category = "Minimap Icon Rendering")
	UMaterialInterface* IconMaterial_UMG;
	// Material to render the icon with to Canvas minimaps
	UPROPERTY(EditAnywhere, Category = "Minimap Icon Rendering")
	UMaterialInterface* IconMaterial_Canvas;
	// Initial icon visibility, can be changed while game is in progress. Note that the plugin handles icon visibility in fog for you.
	UPROPERTY(EditAnywhere, Category = "Minimap Icon Rendering")
	bool bIconVisible = true;
	// Whether the icon rotates or is always rendered with the same orientation
	UPROPERTY(EditAnywhere, Category = "Minimap Icon Rendering")
	bool bIconRotates = false;
	// If screen space, icon will always appear same size no matter how far zoomed. If world space, IconSize represents world diameter and icon will automatically scale when zooming in or out.
	UPROPERTY(EditAnywhere, Category = "Minimap Icon Rendering")
	EIconSizeUnit IconSizeUnit = EIconSizeUnit::ScreenSpace;
	// Render size of icon. Its application depends on the setting of IconSizeUnit.
	UPROPERTY(EditAnywhere, Category = "Minimap Icon Rendering", meta = (ClampMin = "1.0"))
	float IconSize = 32.0f;
	// Color multiplier applied to the icon texture
	UPROPERTY(EditAnywhere, Category = "Minimap Icon Rendering")
	FLinearColor IconDrawColor = FLinearColor::White;
	// Icons with higher ZOrder are rendered on top
	UPROPERTY(EditAnywhere, Category = "Minimap Icon Rendering")
	int32 IconZOrder = 0;

	// Whether the icon is shown on the edge of the minimap if its not within view range
	UPROPERTY(EditAnywhere, Category = "Minimap Objective Arrow")
	bool bObjectiveArrowEnabled = false;
	// When icon is at edge, the icon to show. If not set, will show default icon.
	UPROPERTY(EditAnywhere, Category = "Minimap Objective Arrow", meta = (EditCondition = "bObjectiveArrowEnabled"))
	UTexture2D* ObjectiveArrowTexture;
	// Material to render the objective arrow with to UMG minimaps
	UPROPERTY(EditAnywhere, Category = "Minimap Objective Arrow")
	UMaterialInterface* ObjectiveArrowMaterial_UMG;
	// Material to render the objective arrow with to Canvas minimaps
	UPROPERTY(EditAnywhere, Category = "Minimap Objective Arrow")
	UMaterialInterface* ObjectiveArrowMaterial_Canvas;
	// Whether the icon rotates or is always rendered with the same orientation
	UPROPERTY(EditAnywhere, Category = "Minimap Objective Arrow", meta = (EditCondition = "bObjectiveArrowEnabled"))
	bool bObjectiveArrowRotates = true;
	// Render size of edge icon
	UPROPERTY(EditAnywhere, Category = "Minimap Objective Arrow", meta = (EditCondition = "bObjectiveArrowEnabled", ClampMin = "1.0"))
	float ObjectiveArrowSize = 50.0f;
	
	// Must be enabled to support tooltips and mouse events (OnIconClicked, OnIconHoverStart, etc). Disable to ensure this icon doesn't block other icons' mouse interaction.
	UPROPERTY(EditAnywhere, Category = "Minimap Mouse Interaction")
	bool bIconInteractable = true;
	// Text to display when mouse overing this icon on the minimap, only works in UMG
	UPROPERTY(EditAnywhere, Category = "Minimap Mouse Interaction", meta = (EditCondition = "bIconInteractable"))
	FName IconTooltipText = NAME_None;
	
	// How this icon appears in combination with multi-level backgrounds
	UPROPERTY(EditAnywhere, Category = "Minimap Environment Interaction")
	EIconBackgroundInteraction IconBackgroundInteraction = EIconBackgroundInteraction::AlwaysRender;
	// How this icon appears in combination with fog
	UPROPERTY(EditAnywhere, Category = "Minimap Environment Interaction")
	EIconFogInteraction IconFogInteraction = EIconFogInteraction::AlwaysRenderUnderFog;
	// For some settings of IconFogInteraction, affects how much of the fog must be cleared before the icon appears (1.0 = 100% cleared)
	UPROPERTY(EditAnywhere, Category = "Minimap Environment Interaction")
	float IconFogRevealThreshold = 0.5f;
	// If enabled, actor will be hidden when location is covered in fog
	UPROPERTY(EditAnywhere, Category = "Minimap Environment Interaction")
	bool bHideOwnerInsideFog = false;
	
private:
	// Tracks per view whether the icon is currently rendered in it
	UPROPERTY(Transient)
	TMap<UMapViewComponent*, bool> IsRenderedPerView;
	
	// Backup of the initial material used to render the icon in UMG
	UPROPERTY(Transient)
	UMaterialInterface* InitialIconMaterial_UMG;
	// Backup of the initial material used to render the icon on a UCanvas
	UPROPERTY(Transient)
	UMaterialInterface* InitialIconMaterial_Canvas;
	// Active material instances used to render the icon in UMG
	UPROPERTY(Transient)
	TMap<UUserWidget*, UMaterialInstanceDynamic*> IconMaterialInstances_UMG;
	// Active material instances used to render the icon on a UCanvas
	UPROPERTY(Transient)
	TMap<UMapRendererComponent*, UMaterialInstanceDynamic*> IconMaterialInstances_Canvas;
	UPROPERTY(Transient)
	TMap<UMapRendererComponent*, UMaterialInstanceDynamic*> ObjectiveArrowMaterialInstances_Canvas;
	
	// Start time of the material effect, used to compute the Time parameter for material animations
	float MaterialEffectStartTime = 0;
	// Mouse-over state which is tracked to ensure that a 'start' event can only be followed by an 'end' event and vice versa.
	bool bMouseOverStarted = false;
	
};
