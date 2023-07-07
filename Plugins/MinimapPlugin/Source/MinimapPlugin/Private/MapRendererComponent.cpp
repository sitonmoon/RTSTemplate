// Journeyman's Minimap by ZKShao.

#include "MapRendererComponent.h"
#include "MinimapPluginPrivatePCH.h"
#include "MapFunctionLibrary.h"
#include "MapTrackerComponent.h"
#include "MapIconComponent.h"
#include "MapViewComponent.h"
#include "MapBackground.h"
#include "MapFog.h"
#include "Engine/Canvas.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

UMapRendererComponent::UMapRendererComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Find the default fill material, which fills the map with black before rendering any potential backgrounds
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FillMaterialFinder(TEXT("/MinimapPlugin/Materials/Background/M_Canvas_BackgroundFill"));
	FillMaterial = FillMaterialFinder.Object;
}

void UMapRendererComponent::BeginPlay()
{
	Super::BeginPlay();

	// If a fill material is set, instantiate it now
	if (FillMaterial)
		FillMaterialInstance = UMaterialInstanceDynamic::Create(FillMaterial, this);

	// Find the map tracker component once at game start
	MapTracker = UMapFunctionLibrary::GetMapTracker(this);
	
	// Apply the auto-locate map view setting
	if (!MapView)
		AutoRelocateMapView();
}

void UMapRendererComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Fire buffered mouse hover events
	TickHoverEvents();
}

void UMapRendererComponent::SetAutoLocateMapView(const EMapViewSearchOption InAutoLocateMapView)
{
	AutoLocateMapView = InAutoLocateMapView;

	// If play has begun already, apply the setting immediately
	const UWorld* World = GetWorld();
	if (World && World->HasBegunPlay())
		AutoRelocateMapView();
}

void UMapRendererComponent::DrawToCanvas(UCanvas* Canvas)
{
	if (!Canvas || !MapTracker || !MapView)
		return;

	// Compute what part of the screen to render to
	FVector2D MapTopLeft, MapSize;
	ComputeCanvasRect(Canvas, MapTopLeft, MapSize);

	// Render all minimap elements to the canvas
	RenderToCanvas(Canvas, MapTopLeft, MapSize);

	// Remember the canvas for click events
	LastCanvas = Canvas;
}

bool UMapRendererComponent::HandleClick(const FVector2D& ScreenPosition, const bool bIsLeftMouseButton)
{
	if (!bIsRendered)
		return false;

	// If player is hovering over any icons (this is cached), fire
	// icon click events and then return that the click has been handled
	if (HoveringIcons.Num() > 0)
	{
		for (UMapIconComponent* MapIcon : HoveringIcons)
			MapIcon->ReceiveClicked(bIsLeftMouseButton);
		return true;
	}

	// Otherwise check if the click was within the map
	if (LastCanvas && MapView)
	{
		// Compute the screen space reserved for the map via user settings
		FVector2D MapTopLeft, MapSize;
		ComputeCanvasRect(LastCanvas, MapTopLeft, MapSize);

		// Compute the screen space actually used, taking into account the map's aspect ratio
		FVector2D RenderRegionTopLeft, RenderRegionSize;
		ComputeRenderRegion(MapTopLeft, MapSize, RenderRegionTopLeft, RenderRegionSize);

		if (MapSize.X > 0 && MapSize.Y > 0)
		{
			// Convert screen position to minimap position
			const float U = (ScreenPosition.X - RenderRegionTopLeft.X) / RenderRegionSize.X;
			const float V = (ScreenPosition.Y - RenderRegionTopLeft.Y) / RenderRegionSize.Y;

			// Check whether the screen click actually hit the minimap
			if (UMapFunctionLibrary::DetectIsInView(FVector2D(U, V), FVector2D::ZeroVector, bIsCircular))
			{
				// Convert the minimap position to world position
				FVector WorldPos;
				MapView->DeprojectViewToWorld(U, V, WorldPos);

				// Fire a 'map clicked' event
				OnMapClicked.Broadcast(WorldPos, bIsLeftMouseButton);
				return true;
			}
		}
	}

	return false;
}

void UMapRendererComponent::SetMapView(UMapViewComponent* InMapView)
{
	MapView = InMapView;
}

void UMapRendererComponent::SetIsCircular(const bool bNewIsCircular)
{
	bIsCircular = bNewIsCircular;
}

bool UMapRendererComponent::IsCircular() const
{
	return bIsCircular;
}

void UMapRendererComponent::SetIsRendered(const bool bNewIsRendered)
{
	bIsRendered = bNewIsRendered;
	if (!bIsRendered)
		ClearHoverEvents();
}

bool UMapRendererComponent::IsRendered() const
{
	return bIsRendered;
}

void UMapRendererComponent::SetDrawFrustum(const bool bNewDrawFrustum)
{
	bDrawFrustum = bNewDrawFrustum;
}

bool UMapRendererComponent::GetDrawFrustum() const
{
	return bDrawFrustum;
}

void UMapRendererComponent::SetFrustumFloorDistance(const float NewFrustumFloorDistance)
{
	FrustumFloorDistance = NewFrustumFloorDistance;
}

float UMapRendererComponent::GetFrustumFloorDistance() const
{
	return FrustumFloorDistance;
}

void UMapRendererComponent::SetBackgroundFillColor(const FLinearColor& NewBackgroundFillColor)
{
	BackgroundFillColor = NewBackgroundFillColor;
}

FLinearColor UMapRendererComponent::GetBackgroundFillColor() const
{
	return BackgroundFillColor;
}

void UMapRendererComponent::SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment)
{
	HorizontalAlignment = InHorizontalAlignment;
}

void UMapRendererComponent::SetVerticalAlignment(EVerticalAlignment InVerticalAlignment)
{
	VerticalAlignment = InVerticalAlignment;
}

void UMapRendererComponent::SetMargin(const int32 Left, const int32 Top, const int32 Right, const int32 Bottom)
{
	Margin.Left = Left;
	Margin.Top = Top;
	Margin.Right = Right;
	Margin.Bottom = Bottom;
}

void UMapRendererComponent::SetSize(const int32 Width, const int32 Height)
{
	Size.X = Width;
	Size.Y = Height;
}

inline static bool MapIconZSortPredicate(const UMapIconComponent& A, const UMapIconComponent& B)
{
     return A.GetIconZOrder() < B.GetIconZOrder();
}

inline static bool MapBackgroundZSortPredicate(const AMapBackground& A, const AMapBackground& B)
{
     return A.GetBackgroundZOrder() < B.GetBackgroundZOrder();
}

void UMapRendererComponent::AutoRelocateMapView()
{
	UMapViewComponent* NewMapView = UMapFunctionLibrary::FindMapView(this, AutoLocateMapView);
	if (NewMapView)
	{
		SetMapView(NewMapView);
	}
	else
	{
		FTimerHandle RetryHandle;
		GetOwner()->GetWorldTimerManager().SetTimer(RetryHandle, this, &UMapRendererComponent::AutoRelocateMapView, 0.2f);
	}
}

void UMapRendererComponent::TickHoverEvents()
{
	// Fire 'hover start' events that were detected during the render phase to avoid doing a lot of double work
	// The events are fired during the tick phase, because gameplay logic should not execute during the render phase
	for (UMapIconComponent* MapIcon : BufferedHoverStartEvents)
		MapIcon->ReceiveHoverStart();
	BufferedHoverStartEvents.Empty();

	// Fire 'hover end' events
	for (UMapIconComponent* MapIcon : BufferedHoverEndEvents)
		MapIcon->ReceiveHoverEnd();
	BufferedHoverEndEvents.Empty();
}

void UMapRendererComponent::ClearHoverEvents()
{
	for (UMapIconComponent* MapIcon : HoveringIcons)
		BufferedHoverEndEvents.Add(MapIcon);
	HoveringIcons.Empty();
}

void UMapRendererComponent::MarkOnHoverStart(UMapIconComponent* MapIcon)
{
	if (!HoveringIcons.Contains(MapIcon))
	{
		HoveringIcons.Add(MapIcon);
		BufferedHoverStartEvents.Add(MapIcon);
	}
}

void UMapRendererComponent::MarkOnHoverEnd(UMapIconComponent* MapIcon)
{
	if (HoveringIcons.Contains(MapIcon))
	{
		HoveringIcons.Remove(MapIcon);
		BufferedHoverEndEvents.Add(MapIcon);
	}
}

void UMapRendererComponent::ComputeCanvasRect(UCanvas* Canvas, FVector2D& MapTopLeft, FVector2D& MapSize)
{
	// Manually apply DPI scale, so that UMG and Canvas settings output the same
	const float DPIScale = UWidgetLayoutLibrary::GetViewportScale(this);

	// Compute horizontal boundaries
	float MinX, MaxX;
	switch (HorizontalAlignment)
	{
	case EHorizontalAlignment::HAlign_Left:
		MinX = DPIScale * Margin.Left;
		MaxX = MinX + DPIScale * Size.X;
		break;
	case EHorizontalAlignment::HAlign_Center:
		MinX = (Canvas->ClipX - DPIScale * Size.X) / 2;
		MaxX = (Canvas->ClipX + DPIScale * Size.X) / 2;
		break;
	case EHorizontalAlignment::HAlign_Right:
		MinX = Canvas->ClipX - DPIScale * Margin.Right - DPIScale * Size.X;
		MaxX = Canvas->ClipX - DPIScale * Margin.Right;
		break;
	case EHorizontalAlignment::HAlign_Fill:
	default:
		MinX = DPIScale * Margin.Left;
		MaxX = Canvas->ClipX - DPIScale * Margin.Right;
		break;
	}
	
	// Compute vertical boundaries
	float MinY, MaxY;
	switch (VerticalAlignment)
	{
	case EVerticalAlignment::VAlign_Top:
		MinY = DPIScale * Margin.Top;
		MaxY = MinY + DPIScale * Size.Y;
		break;
	case EVerticalAlignment::VAlign_Center:
		MinY = (Canvas->ClipY - DPIScale * Size.Y) / 2;
		MaxY = (Canvas->ClipY + DPIScale * Size.Y) / 2;
		break;
	case EVerticalAlignment::VAlign_Bottom:
		MinY = Canvas->ClipY - DPIScale * Margin.Bottom - DPIScale * Size.Y;
		MaxY = Canvas->ClipY - DPIScale * Margin.Bottom;
		break;
	case EVerticalAlignment::VAlign_Fill:
	default:
		MinY = DPIScale * Margin.Top;
		MaxY = Canvas->ClipY - DPIScale * Margin.Bottom;
		break;
	}
	
	// Convert to top left and size
	MapTopLeft = FVector2D(MinX, MinY);
	MapSize = FVector2D(MaxX, MaxY) - MapTopLeft;
}

void UMapRendererComponent::ComputeRenderRegion(const FVector2D& MapTopLeft, const FVector2D& MapSize, FVector2D& RenderRegionTopLeft, FVector2D& RenderRegionSize)
{
	// Compute best fit, using the view's aspect ratio
	RenderRegionTopLeft = MapTopLeft;
	RenderRegionSize = MapSize;
	const FVector2D MapScreenCenter = MapTopLeft + 0.5f * MapSize;
	const float MapAspectRatio = MapSize.X / MapSize.Y;
	const float ViewAspectRatio = bIsCircular ? 1.0f : MapView->GetViewAspectRatio();
	if (MapAspectRatio != ViewAspectRatio)
	{
		const float CandidateMapWidth = ViewAspectRatio * MapSize.Y;
		const float CandidateMapHeight = MapSize.X / ViewAspectRatio;
		const FVector2D NewMapScale = (CandidateMapWidth > MapSize.X) ? FVector2D(1.0f, MapSize.X / CandidateMapWidth) : FVector2D(MapSize.Y / CandidateMapHeight, 1.0f);
		const FVector2D NewMapSize = MapSize * NewMapScale;
		RenderRegionTopLeft = MapScreenCenter - 0.5f * NewMapSize;
		RenderRegionSize = NewMapSize;
	}
}

void UMapRendererComponent::RenderToCanvas(UCanvas* Canvas, const FVector2D& MapTopLeft, const FVector2D& MapSize)
{
	if (!Canvas || !MapTracker || !MapView || !bIsRendered)
		return;

	Canvas->Reset();
	Canvas->SetDrawColor(FColor(255, 255, 255, 255));

	// If aspect ratio of view and rendering region don't match, resize the rendering region
	FVector2D RenderRegionTopLeft, RenderRegionSize;
	ComputeRenderRegion(MapTopLeft, MapSize, RenderRegionTopLeft, RenderRegionSize);
	
	// Draw layers from back to front
	DrawBackground(Canvas, RenderRegionTopLeft, RenderRegionSize);
	DrawIcons(Canvas, RenderRegionTopLeft, RenderRegionSize, false);
	DrawFog(Canvas, RenderRegionTopLeft, RenderRegionSize);
	DrawIcons(Canvas, RenderRegionTopLeft, RenderRegionSize, true);
	DrawBoundary(Canvas, RenderRegionTopLeft, RenderRegionSize);
	DrawFrustum(Canvas, RenderRegionTopLeft, RenderRegionSize);
}

void UMapRendererComponent::DrawBackground(UCanvas* Canvas, const FVector2D& RenderRegionTopLeft, const FVector2D& RenderRegionSize)
{
	const FVector2D RenderRegionCenter = RenderRegionTopLeft + 0.5f * RenderRegionSize;
	const FVector2D RenderRegionBottomRight = RenderRegionTopLeft + RenderRegionSize;

	// Draw opaque base background
	if (FillMaterialInstance && BackgroundFillColor.A > 0)
	{
		TArray<FVector2D> CornerUVs;
		CornerUVs.Add(FVector2D(0, 0));
		CornerUVs.Add(FVector2D(1, 0));
		CornerUVs.Add(FVector2D(1, 1));
		CornerUVs.Add(FVector2D(0, 1));

		// Prepare background triangles
		FCanvasUVTri Tri1;
		Tri1.V0_Pos = RenderRegionTopLeft;
		Tri1.V1_Pos = RenderRegionTopLeft + FVector2D(RenderRegionSize.X, 0);
		Tri1.V2_Pos = RenderRegionTopLeft + FVector2D(0, RenderRegionSize.Y);
		Tri1.V0_UV = CornerUVs[0];
		Tri1.V1_UV = CornerUVs[1];
		Tri1.V2_UV = CornerUVs[3];
		Tri1.V0_Color = BackgroundFillColor;
		Tri1.V1_Color = BackgroundFillColor;
		Tri1.V2_Color = BackgroundFillColor;

		FCanvasUVTri Tri2;
		Tri2.V0_Pos = RenderRegionTopLeft + FVector2D(RenderRegionSize.X, 0);
		Tri2.V1_Pos = RenderRegionTopLeft + FVector2D(0, RenderRegionSize.Y);
		Tri2.V2_Pos = RenderRegionTopLeft + RenderRegionSize;
		Tri2.V0_UV = CornerUVs[1];
		Tri2.V1_UV = CornerUVs[3];
		Tri2.V2_UV = CornerUVs[2];
		Tri2.V0_Color = BackgroundFillColor;
		Tri2.V1_Color = BackgroundFillColor;
		Tri2.V2_Color = BackgroundFillColor;

		// Push clip parameters and render background
		FillMaterialInstance->SetVectorParameterValue(TEXT("ClipInfo"), FLinearColor(RenderRegionCenter.X, RenderRegionCenter.Y, RenderRegionSize.X, !bIsCircular ? RenderRegionSize.Y : -1));
		Canvas->K2_DrawMaterialTriangle(FillMaterialInstance, { Tri1, Tri2 });
	}

	// Collect rendered backgrounds
	bool bUsingPriorityBackground;
	const int32 ActiveBackgroundPriority = MapView->GetActiveBackgroundPriority(bUsingPriorityBackground);
	const TArray<AMapBackground*> MapBackgrounds = MapTracker->GetMapBackgrounds();
	TArray<AMapBackground*> ShownMapBackgrounds;
	for (AMapBackground* MapBackground : MapBackgrounds)
	{
		UTexture* BackgroundTexture = MapBackground->GetBackgroundTexture();
		if (!BackgroundTexture)
			continue;
		if (!MapBackground->IsBackgroundVisible() || (bUsingPriorityBackground && MapBackground->GetBackgroundPriority() != ActiveBackgroundPriority))
			continue;
		ShownMapBackgrounds.Add(MapBackground);
	}

	// Sort rendered backgrounds back to front
	ShownMapBackgrounds.Sort(MapBackgroundZSortPredicate);
	
	// Draw backgrounds back to front
	for (AMapBackground* MapBackground : ShownMapBackgrounds)
	{
		// Compute UV coordinates of visible part of the background. Skip if map view doesn't intersect the area.
		TArray<FVector2D> CornerUVs;
		if (!MapBackground->GetMapViewCornerUVs(MapView, CornerUVs))
			continue;
		
		// Retrieve the material used to render the background. Skip if no material is set.
		UMaterialInstanceDynamic* MatInst = MapBackground->GetBackgroundMaterialInstanceForCanvas(this);
		if (!MatInst)
			return;

		// If this is a multi-level background, refresh the active texture
		if (MapBackground->IsMultiLevel())
		{
			const int32 Level = MapView->GetActiveBackgroundLevel(MapBackground);
			MatInst->SetTextureParameterValue(TEXT("Texture"), MapBackground->GetBackgroundTexture(Level));
		}

		// Prepare background triangles
		FCanvasUVTri Tri1;
		Tri1.V0_Pos = RenderRegionTopLeft;
		Tri1.V1_Pos = RenderRegionTopLeft + FVector2D(RenderRegionSize.X, 0);
		Tri1.V2_Pos = RenderRegionTopLeft + FVector2D(0, RenderRegionSize.Y);
		Tri1.V0_UV = CornerUVs[0];
		Tri1.V1_UV = CornerUVs[1];
		Tri1.V2_UV = CornerUVs[3];
		Tri1.V0_Color = FLinearColor::White;
		Tri1.V1_Color = FLinearColor::White;
		Tri1.V2_Color = FLinearColor::White;

		FCanvasUVTri Tri2;
		Tri2.V0_Pos = RenderRegionTopLeft + FVector2D(RenderRegionSize.X, 0);
		Tri2.V1_Pos = RenderRegionTopLeft + FVector2D(0, RenderRegionSize.Y);
		Tri2.V2_Pos = RenderRegionTopLeft + RenderRegionSize;
		Tri2.V0_UV = CornerUVs[1];
		Tri2.V1_UV = CornerUVs[3];
		Tri2.V2_UV = CornerUVs[2];
		Tri2.V0_Color = FLinearColor::White;
		Tri2.V1_Color = FLinearColor::White;
		Tri2.V2_Color = FLinearColor::White;

		// Render background using material
		MatInst->SetVectorParameterValue(TEXT("ClipInfo"), FLinearColor(RenderRegionCenter.X, RenderRegionCenter.Y, RenderRegionSize.X, !bIsCircular ? RenderRegionSize.Y : -1));
		Canvas->K2_DrawMaterialTriangle(MatInst, { Tri1, Tri2 });
	}
}

void UMapRendererComponent::DrawFog(UCanvas* Canvas, const FVector2D& RenderRegionTopLeft, const FVector2D& RenderRegionSize)
{
	const TArray<AMapFog*> MapFogs = MapTracker->GetMapFogs();
	for (AMapFog* MapFog : MapFogs)
	{
		// Compute UV coordinates of visible part of the fog. Skip if map view doesn't intersect the area.
		TArray<FVector2D> CornerUVs;
		if (!MapFog->GetMapViewCornerUVs(MapView, CornerUVs))
			continue;
		
		// Retrieve the material used to render the fog. Skip if no material is set.
		UMaterialInstanceDynamic* MatInst = MapFog->GetFogMaterialInstanceForCanvas(this);
		if (!MatInst)
			continue;

		// Prepare background triangles
		FCanvasUVTri Tri1;
		Tri1.V0_Pos = RenderRegionTopLeft;
		Tri1.V1_Pos = RenderRegionTopLeft + FVector2D(RenderRegionSize.X, 0);
		Tri1.V2_Pos = RenderRegionTopLeft + FVector2D(0, RenderRegionSize.Y);
		Tri1.V0_UV = CornerUVs[0];
		Tri1.V1_UV = CornerUVs[1];
		Tri1.V2_UV = CornerUVs[3];
		Tri1.V0_Color = FLinearColor::White;
		Tri1.V1_Color = FLinearColor::White;
		Tri1.V2_Color = FLinearColor::White;

		FCanvasUVTri Tri2;
		Tri2.V0_Pos = RenderRegionTopLeft + FVector2D(RenderRegionSize.X, 0);
		Tri2.V1_Pos = RenderRegionTopLeft + FVector2D(0, RenderRegionSize.Y);
		Tri2.V2_Pos = RenderRegionTopLeft + RenderRegionSize;
		Tri2.V0_UV = CornerUVs[1];
		Tri2.V1_UV = CornerUVs[3];
		Tri2.V2_UV = CornerUVs[2];
		Tri2.V0_Color = FLinearColor::White;
		Tri2.V1_Color = FLinearColor::White;
		Tri2.V2_Color = FLinearColor::White;

		// Render background using material
		const FVector2D RenderRegionCenter = RenderRegionTopLeft + 0.5f * RenderRegionSize;
		MatInst->SetVectorParameterValue(TEXT("ClipInfo"), FLinearColor(RenderRegionCenter.X, RenderRegionCenter.Y, RenderRegionSize.X, !bIsCircular ? RenderRegionSize.Y : -1));
		Canvas->K2_DrawMaterialTriangle(MatInst, { Tri1, Tri2 });
	}
}

void UMapRendererComponent::DrawIcons(UCanvas* Canvas, const FVector2D& RenderRegionTopLeft, const FVector2D& RenderRegionSize, const bool bAboveFog)
{
	// Not using #define here because it may interfere with end user #defines.
	const float DPIScale = UWidgetLayoutLibrary::GetViewportScale(this);
	static const float ICONSIZE_TO_INNERRADIUS = 0.5f;
	static const float ICONSIZE_TO_OUTERRADIUS = 0.5f * FMath::Sqrt(2.0f);
	
	const FVector2D RenderRegionCenter = RenderRegionTopLeft + 0.5f * RenderRegionSize;
	const FVector2D RenderRegionBottomRight = RenderRegionTopLeft + RenderRegionSize;

	FVector2D MousePosition;
	APlayerController* FirstPC = GetWorld()->GetFirstPlayerController();
	if (FirstPC)
		FirstPC->GetMousePosition(MousePosition.X, MousePosition.Y);

	// Determine icons in view
	float ViewExtentX, ViewExtentY;
	MapView->GetViewExtent(ViewExtentX, ViewExtentY);

	const TArray<UMapIconComponent*>& MapIcons = MapTracker->GetMapIcons();
	const FVector2D UVToPixelRatio = FVector2D::UnitVector / RenderRegionSize;
	const float WorldToPixelRatio = 2.0f * ViewExtentX * UVToPixelRatio.X;
	const float PixelToWorldRatio = 1.0f / WorldToPixelRatio;
	TArray<UMapIconComponent*> MapIconsInView;
	for (UMapIconComponent* MapIcon : MapIcons)
	{
		// Ignore hidden map icons
		if (!MapIcon->IsIconVisible())
			continue;

		// Ignore icon with invalid size
		const float IconSize = MapIcon->GetIconSize() * (MapIcon->GetIconSizeUnit() == EIconSizeUnit::WorldSpace ? PixelToWorldRatio : DPIScale);
		if (IconSize <= 0.0f)
			continue;

		// Ignore icon if no material set
		if (!MapIcon->GetIconMaterialForCanvas() && !MapIcon->GetObjectiveArrowMaterialForCanvas())
			continue;

		// Ignore icons that are on another layer with respect to fog
		const EIconFogInteraction FogInteraction = MapIcon->GetIconFogInteraction();
		if (bAboveFog && FogInteraction == EIconFogInteraction::AlwaysRenderUnderFog)
			continue;

		// Do a fast check that eliminates most icons that aren't in the view
		// Takes into account the icon's size
		const float IconWorldRadius = IconSize * ICONSIZE_TO_OUTERRADIUS * WorldToPixelRatio;
		if (!MapIcon->IsObjectiveArrowEnabled() && !MapView->ViewContains(MapIcon->GetComponentLocation(), IconWorldRadius))
		{
			MapIcon->MarkRenderedInView(MapView, false);
			MarkOnHoverEnd(MapIcon);
			continue;
		}

		// Eliminate icons that are in the same mult-level background volume, but not in the same level
		if (!MapView->IsSameBackgroundLevel(MapIcon))
			continue;
		
		// Check whether the icon is visible in the fog
		if (MapTracker->HasMapFog())
		{
			switch (FogInteraction)
			{
			case EIconFogInteraction::OnlyRenderWhenExplored:
			case EIconFogInteraction::OnlyRenderWhenRevealing:
				const FVector WorldLocation = MapIcon->GetComponentLocation();
				const bool RequireCurrentlySeeing = FogInteraction == EIconFogInteraction::OnlyRenderWhenRevealing;
				const float FogRevealThreshold = MapIcon->GetIconFogRevealThreshold();
				bool bIsInsideFogVolume;
				if (MapTracker->GetFogRevealedFactor(WorldLocation, RequireCurrentlySeeing, bIsInsideFogVolume) < FogRevealThreshold)
					continue;
				break;
			}
		}

		// Icon passed all checks, schedule it for rendering
		MapIconsInView.Add(MapIcon);
	}

	// Sort icons on ZOrder
	MapIconsInView.Sort(MapIconZSortPredicate);

	// Draw icons in view
	for (UMapIconComponent* MapIcon : MapIconsInView)
	{
		// Compute view coordinates
		float U, V, Yaw;
		MapView->GetViewCoordinates(MapIcon->GetComponentLocation(), bIsCircular, U, V);
		MapView->GetViewYaw(MapIcon->GetComponentRotation().Yaw, Yaw);

		// Compute icon radius. Icon is rectangular. For some purposes we use the inner radius,
		// while for other purposes we use the outer radius that includes the corners.
		float IconSize = MapIcon->GetIconSize() * (MapIcon->GetIconSizeUnit() == EIconSizeUnit::WorldSpace ? PixelToWorldRatio : DPIScale);
		float IconInnerRadius = IconSize * ICONSIZE_TO_INNERRADIUS;
		float IconOuterRadius = IconSize * ICONSIZE_TO_OUTERRADIUS;

		// Determine whether the icon is within view. Use outer radius because as long 
		// as any corner is in view, the icon should be rendered.
		bool IsWithinView = UMapFunctionLibrary::DetectIsInView(FVector2D(U, V), UVToPixelRatio * IconOuterRadius, bIsCircular);
		MapIcon->MarkRenderedInView(MapView, IsWithinView);

		// If icon should appear at border, update the UV coordinates
		float EdgeYaw = 0.0f;
		if (!IsWithinView && MapIcon->IsObjectiveArrowEnabled())
		{
			// Use edge icon size instead
			IconSize = MapIcon->GetObjectiveArrowSize() * DPIScale;
			IconInnerRadius = IconSize * ICONSIZE_TO_INNERRADIUS;
			IconOuterRadius = IconSize * ICONSIZE_TO_OUTERRADIUS;

			// Compute the direction towards the world object
			EdgeYaw = FMath::RadiansToDegrees(FMath::Atan2(V - 0.5f, U - 0.5f));
			
			// Ensure entire icon is within boundaries
			const FVector2D UVClamped = UMapFunctionLibrary::ClampIntoView(FVector2D(U, V), 0.5f * UVToPixelRatio.X * IconInnerRadius, bIsCircular);
			U = UVClamped.X;
			V = UVClamped.Y;
		}

		// Retrieve icon texture (can be null), material (can be null) and color
		const bool IsShowingEdgeIcon = !IsWithinView && MapIcon->GetObjectiveArrowTexture() != nullptr && MapIcon->IsObjectiveArrowEnabled();
		UTexture* Icon = IsShowingEdgeIcon ? MapIcon->GetObjectiveArrowTexture() : MapIcon->GetIconTexture();
		UMaterialInstanceDynamic* MatInst = IsShowingEdgeIcon ? MapIcon->GetObjectiveArrowMaterialInstanceForCanvas(this) : MapIcon->GetIconMaterialInstanceForCanvas(this);
		if (!MatInst)
			continue;;

		const FLinearColor& IconDrawColor = MapIcon->GetIconDrawColor();
		
		// Finalize icon render position
		const FVector2D IconScreenPos = RenderRegionTopLeft + FVector2D(U, V) * RenderRegionSize;
		const FVector2D IconScreenSize(IconSize, IconSize);
		const FVector2D IconTopLeft = IconScreenPos - 0.5f * IconScreenSize;
		
		// Finalize icon's render rotation
		float RenderYaw = 0.0f;
		if (IsShowingEdgeIcon)
		{
			// Make icon point towards world object
			if (MapIcon->DoesObjectiveArrowRotate())
				RenderYaw = EdgeYaw;
		}
		else
		{
			// Apply relative rotation
			if (MapIcon->DoesIconRotate())
				RenderYaw = Yaw;
		}
		
		if (MapIcon->IsIconInteractable())
		{
			// Compute whether cursor is on icon
			bool IsMouseOvering = false;
			if (IsWithinView)
			{
				const float MDX = MousePosition.X - IconScreenPos.X;
				const float MDY = MousePosition.Y - IconScreenPos.Y;
				IsMouseOvering = FMath::Pow(MDX, 2) + FMath::Pow(MDY, 2) < FMath::Pow(0.5f * IconSize, 2);
			}

			// Schedule an event if a change is detected
			if (IsMouseOvering)
				MarkOnHoverStart(MapIcon);
			else
				MarkOnHoverEnd(MapIcon);
		}
		
		// Push clip parameters
		MatInst->SetTextureParameterValue(TEXT("Texture"), Icon);
		MatInst->SetVectorParameterValue(TEXT("ClipInfo"), FLinearColor(RenderRegionCenter.X, RenderRegionCenter.Y, RenderRegionSize.X, !bIsCircular ? RenderRegionSize.Y : -1));
		MatInst->SetVectorParameterValue(TEXT("Color"), IconDrawColor);
		
		// Compute rotated quad triangles. Since icons are square, we can compute the delta from center of
		// one of the corners and then repeatedly rotate that delta by 90 degrees, which can be done
		// by flipping the X and Y deltas.
		const FVector2D HalfIconScreenSize(0.5f * IconScreenSize);
		const FVector2D CornerDelta = HalfIconScreenSize.GetRotated(RenderYaw);

		FCanvasUVTri Tri1;
		Tri1.V0_Pos = IconScreenPos + FVector2D(-CornerDelta.X, -CornerDelta.Y);
		Tri1.V1_Pos = IconScreenPos + FVector2D(CornerDelta.Y, -CornerDelta.X);
		Tri1.V2_Pos = IconScreenPos + FVector2D(-CornerDelta.Y, CornerDelta.X);
		Tri1.V0_UV = FVector2D(0, 0);
		Tri1.V1_UV = FVector2D(1, 0);
		Tri1.V2_UV = FVector2D(0, 1);
		
		FCanvasUVTri Tri2;
		Tri2.V0_Pos = IconScreenPos + FVector2D(CornerDelta.Y, -CornerDelta.X);
		Tri2.V1_Pos = IconScreenPos + FVector2D(-CornerDelta.Y, CornerDelta.X);
		Tri2.V2_Pos = IconScreenPos + FVector2D(CornerDelta.X, CornerDelta.Y);
		Tri2.V0_UV = FVector2D(1, 0);
		Tri2.V1_UV = FVector2D(0, 1);
		Tri2.V2_UV = FVector2D(1, 1);

		// Draw the material quad
		Canvas->K2_DrawMaterialTriangle(MatInst, { Tri1, Tri2 });
	}
}

void UMapRendererComponent::DrawBoundary(UCanvas* Canvas, const FVector2D& RenderRegionTopLeft, const FVector2D& RenderRegionSize)
{
	const FVector2D RenderRegionCenter = RenderRegionTopLeft + 0.5f * RenderRegionSize;
	const FVector2D RenderRegionBottomRight = RenderRegionTopLeft + RenderRegionSize;

	// Draw boundary
	if (bIsCircular)
	{
		// Draw circle boundary
		static const uint8 NumSegments = 64;
		static const float DeltaAngle = 2.0f * PI / float(NumSegments);
		const float Radius = 0.5f * RenderRegionSize.X;
		float Angle = 0.0f;
		FVector2D LastPoint = RenderRegionCenter + FVector2D(Radius, 0);
		for (uint8 i = 0; i < NumSegments; ++i)
		{
			const FVector2D A = LastPoint;
			const FVector2D B = RenderRegionCenter + FVector2D(FMath::Cos(Angle + DeltaAngle) * Radius, FMath::Sin(Angle + DeltaAngle) * Radius);
			Canvas->K2_DrawLine(A, B, 2.0f, FLinearColor::Black);
			LastPoint = B;
			Angle += DeltaAngle;
		}
	}
	else
	{
		// Draw rectangle boundary
		Canvas->K2_DrawLine(RenderRegionTopLeft, RenderRegionTopLeft + FVector2D(RenderRegionSize.X, 0), 2.0f, FLinearColor::Black);
		Canvas->K2_DrawLine(RenderRegionTopLeft, RenderRegionTopLeft + FVector2D(0, RenderRegionSize.Y), 2.0f, FLinearColor::Black);
		Canvas->K2_DrawLine(RenderRegionBottomRight, RenderRegionTopLeft + FVector2D(RenderRegionSize.X, 0), 2.0f, FLinearColor::Black);
		Canvas->K2_DrawLine(RenderRegionBottomRight, RenderRegionTopLeft + FVector2D(0, RenderRegionSize.Y), 2.0f, FLinearColor::Black);
	}
}

void UMapRendererComponent::DrawFrustum(UCanvas* Canvas, const FVector2D& RenderRegionTopLeft, const FVector2D& RenderRegionSize)
{
	// Draw frustum
	if (bDrawFrustum)
	{
		TArray<FVector2D> CornerUVs;
		if (UMapFunctionLibrary::ComputeViewFrustum(this, MapView, bIsCircular, CornerUVs))
		{
			// Convert to screen coordinates
			TArray<FVector2D> ScreenPoints;
			for (const FVector2D& WorldCorner : CornerUVs)
				ScreenPoints.Add(RenderRegionTopLeft + WorldCorner * RenderRegionSize);

			// Draw lines
			ScreenPoints.Add(FVector2D(ScreenPoints[0]));
			for (uint8 i = 0; i < 4; ++i)
				Canvas->K2_DrawLine(ScreenPoints[i], ScreenPoints[i + 1], 1.0f, FLinearColor::White);
		}
	}
}
