// Journeyman's Minimap by ZKShao.

#include "MapViewComponent.h"
#include "MinimapPluginPrivatePCH.h"
#include "MapIconComponent.h"
#include "MapTrackerComponent.h"
#include "MapBackground.h"
#include "MapFunctionLibrary.h"

UMapViewComponent::UMapViewComponent()
{
	SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
	BoxExtent = FVector(1000, 1000, 1);
	LastTransform = FTransform::Identity;
	CachedInverseTransform = FTransform::Identity;
}

#if WITH_EDITOR
bool UMapViewComponent::CanEditChange(const FProperty* InProperty) const
{
	const bool bParentVal = Super::CanEditChange(InProperty);
	if (!bParentVal)
		return false;

	if (InProperty->GetName() == "FixedRotation")
		return RotationMode == EMapViewRotationMode::UseFixedRotation;
	else if (InProperty->GetName() == "InheritedYawOffset")
		return RotationMode == EMapViewRotationMode::InheritYaw;
	return true;
}
#endif

void UMapViewComponent::BeginPlay()
{
	Super::BeginPlay();

	// Apply the initial view size to initialize precomputed values
	const FVector ScaledBoxExtent = GetScaledBoxExtent();
	SetViewExtent(ScaledBoxExtent.X, ScaledBoxExtent.Y);
	SetZoomScale(1.0f);

	UMapTrackerComponent* MapTracker = UMapFunctionLibrary::GetMapTracker(this);
	if (MapTracker) 
	{
		MapTracker->OnMapBackgroundRegistered.AddDynamic(this, &UMapViewComponent::RegisterMultiLevelMapBackground);
		MapTracker->OnMapBackgroundUnregistered.AddDynamic(this, &UMapViewComponent::UnregisterMultiLevelMapBackground);

		for (AMapBackground* MapBackground : MapTracker->GetMapBackgrounds())
			RegisterMultiLevelMapBackground(MapBackground);
	}
}

void UMapViewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	UMapTrackerComponent* MapTracker = UMapFunctionLibrary::GetMapTracker(this);
	if (MapTracker) 
	{
		MapTracker->OnMapBackgroundRegistered.RemoveDynamic(this, &UMapViewComponent::RegisterMultiLevelMapBackground);
		MapTracker->OnMapBackgroundUnregistered.RemoveDynamic(this, &UMapViewComponent::UnregisterMultiLevelMapBackground);
	}
	
	// Let listeners (like a UMG widget) know this map view is destroyed
	OnViewDestroyed.Broadcast(this);
}

void UMapViewComponent::SetIconCategoryVisible(FName IconCategory, const bool bNewVisible)
{
	if (IconCategory == NAME_None)
		return;
	if (bNewVisible && HiddenIconCategories.Contains(IconCategory))
	{
		HiddenIconCategories.Remove(IconCategory);
		OnVisibleCategoriesChanged.Broadcast(this);
	}
	else if (!bNewVisible && !HiddenIconCategories.Contains(IconCategory))
	{
		HiddenIconCategories.Add(IconCategory);
		OnVisibleCategoriesChanged.Broadcast(this);
	}
}

bool UMapViewComponent::IsIconCategoryVisible(FName IconCategory) const
{
	return IconCategory == NAME_None || !HiddenIconCategories.Contains(IconCategory);
}

void UMapViewComponent::SetViewExtent(const float NewViewExtentX, const float NewViewExtentY)
{
	// Update box extent which will represent the view size graphically and data-wise
	const float ViewExtentX = FMath::Max(0.01f, NewViewExtentX);
	const float ViewExtentY = FMath::Max(0.01f, NewViewExtentY);
	SetBoxExtent(FVector(ViewExtentX, ViewExtentY, 1), true);
	UpdateViewSize();
}

void UMapViewComponent::GetViewExtent(float& ViewExtentX, float& ViewExtentY) const
{
	const FVector ScaledExtents = GetScaledBoxExtent();
	ViewExtentX = ScaledExtents.X;
	ViewExtentY = ScaledExtents.Y;
}

void UMapViewComponent::SetZoomScale(const float NewZoomScale)
{
	SetWorldScale3D(FVector(FMath::Max<float>(KINDA_SMALL_NUMBER, NewZoomScale)));
	UpdateViewSize();
}

float UMapViewComponent::GetZoomScale() const
{
	return GetComponentScale().X;
}

float UMapViewComponent::GetViewAspectRatio() const
{
	const FVector ScaledBoxExtent = GetScaledBoxExtent();
	return (ScaledBoxExtent.Y > 0) ? ScaledBoxExtent.X / ScaledBoxExtent.Y : 1.0f;
}

TArray<FVector> UMapViewComponent::GetWorldCorners()
{
	UpdateTransformCache();

	// Retrieve location and rotation
	const FVector Center(GetComponentLocation());
	const FVector ScaledBoxExtent = GetScaledBoxExtent();
	const FVector2D ScaledBoxExtent2D(ScaledBoxExtent.X, ScaledBoxExtent.Y);
			
	// Generate the corners
	TArray<FVector> Corners;
	Corners.SetNumZeroed(4);
	const static TArray<FVector2D> CornerScales{FVector2D(-1, -1), FVector2D(1, -1)};
	for (uint8 i = 0; i < 2; ++i)
	{
		// Define corner in local space, then transform to world space using location and rotation
		const FVector2D LocalCorner = CornerScales[i] * ScaledBoxExtent2D;
		const FVector RotatedCorner3D = CachedTransform.Rotator().RotateVector(FVector(LocalCorner.X, LocalCorner.Y, 0));

		// Use the fact that the view box is symmetrical to save some computations
		Corners[i] = Center + RotatedCorner3D;
		Corners[i+2] = Center - RotatedCorner3D;
	}

	return Corners;
}

bool UMapViewComponent::ViewContains(const FVector& WorldPos, const float WorldRadius) const
{
	// Compute whether an object in the world is in view. The object and the view box are approximated as circles. 
	// This check is designed to be cheap, to broadly eliminate actors that are guaranteed not in view.
	const FVector ScaledBoxExtent = GetScaledBoxExtent();
	const float ViewRadiusSquared = FMath::Pow(ScaledBoxExtent.X + WorldRadius, 2.0f) + FMath::Pow(ScaledBoxExtent.Y + WorldRadius, 2.0f);
	const float DistanceSquared = FVector::DistSquaredXY(WorldPos, GetComponentLocation());
	return DistanceSquared < ViewRadiusSquared;
}

bool UMapViewComponent::GetViewCoordinates(const FVector& WorldPos, bool bForceRectangular, float& U, float& V)
{
	UpdateTransformCache();

	// Convert world location to local space and normalize from (-1.0, 1.0) to (0.0, 1.0)
	const FVector LocalPos = CachedInverseTransform.TransformPosition(WorldPos);
	U = 0.5f + ((bForceRectangular ? InverseViewRadius : CachedInverseViewSize.X) * LocalPos.X);
	V = 0.5f + ((bForceRectangular ? InverseViewRadius : CachedInverseViewSize.Y) * LocalPos.Y);

	// Return if the position is in view
	return !(U < 0 || U > 1 || V < 0 || V > 1);
}

void UMapViewComponent::GetViewYaw(const float WorldYaw, float& Yaw)
{
	UpdateTransformCache();

	// Compute relative angle
	const float ReferenceYaw = CachedTransform.Rotator().Yaw;
	Yaw = WorldYaw - ReferenceYaw;
}

void UMapViewComponent::DeprojectViewToWorld(const float U, const float V, FVector& WorldPos)
{
	UpdateTransformCache();

	// Transforms minimap coordinates to world coordinates.
	const FVector UnscaledBoxExtent = GetUnscaledBoxExtent();
	const float LocalX = (U - 0.5f) * 2.0f * UnscaledBoxExtent.X;
	const float LocalY = (V - 0.5f) * 2.0f * UnscaledBoxExtent.Y;
	WorldPos = CachedTransform.TransformPosition(FVector(LocalX, LocalY, 0));
}

int32 UMapViewComponent::GetActiveBackgroundPriority(bool& IsInsideAnyBackground)
{
	UpdateBackgroundCache();
	IsInsideAnyBackground = bInsideAnyBackground;
	return BackgroundPriority;
}

int32 UMapViewComponent::GetActiveBackgroundLevel(const AMapBackground* MapBackground)
{
	UpdateBackgroundCache();
	return PositionOnMultiLevelBackgrounds.Contains(MapBackground) ? PositionOnMultiLevelBackgrounds[MapBackground] : 0;
}

bool UMapViewComponent::IsSameBackgroundLevel(const UMapIconComponent* MapIcon)
{
	if (!MapIcon)
		return false;
	
	const EIconBackgroundInteraction BackgroundInteraction = MapIcon->GetIconBackgroundInteraction();
	if (BackgroundInteraction == EIconBackgroundInteraction::AlwaysRender)
		return true;

	UpdateBackgroundCache();

	// If the player isn't inside a multi-level background, no background is preventing the icon from rendering
	if (!bInsideAnyBackground)
		return true;
	
	const FVector MapIconPos = MapIcon->GetComponentLocation();
	TSet<AMapBackground*> SurroundingBackgrounds;
	for (AMapBackground* Background : MapBackgrounds)
	{
		// Skip this background if its not rendered
		if (!Background->IsBackgroundVisible())
			continue;

		// Icon must satisfy requirements for any background that the view is in
		if (Background->GetMapView()->ViewContains(MapIconPos, 0.0f))
			SurroundingBackgrounds.Add(Background);
	}

	if (SurroundingBackgrounds.Num() == 0)
		return true;
	
	const bool bRequireHighestPriority = BackgroundInteraction == EIconBackgroundInteraction::OnlyRenderInPriorityVolume || BackgroundInteraction == EIconBackgroundInteraction::OnlyRenderOnPriorityFloor;
	int32 RequiredBackgroundPriority = INT_MIN;
	if (bRequireHighestPriority)
	{
		for (AMapBackground* Background : SurroundingBackgrounds)
		{
			RequiredBackgroundPriority = FMath::Max(RequiredBackgroundPriority, Background->GetBackgroundPriority());
		}
	}

	for (AMapBackground* Background : SurroundingBackgrounds)
	{
		// Skip backgrounds with too low priority
		if (RequiredBackgroundPriority > INT_MIN && Background->GetBackgroundPriority() != RequiredBackgroundPriority)
			continue;

		// Skip backgrounds not being rendered, because we're inside a higher priority background
		if (Background->GetBackgroundPriority() != BackgroundPriority)
			continue;

		// If background is not multi-level, the icon is trivially on the same level, so render it
		if (!Background->IsMultiLevel())
			return true;
		
		switch (BackgroundInteraction)
		{
		case EIconBackgroundInteraction::OnlyRenderInSameVolume:
			// If icon does not have to be on the same level, render it
			return true;
		case EIconBackgroundInteraction::OnlyRenderOnSameFloor:
			// If the icon is on the same level, render it
			if (Background->GetLevelAtHeight(MapIconPos.Z) == PositionOnMultiLevelBackgrounds[Background])
				return true;
			break;
		}
	}

	return false;
}

void UMapViewComponent::RegisterMultiLevelMapBackground(AMapBackground* MapBackground)
{
	MapBackgrounds.Add(MapBackground);
	if (MapBackground->IsMultiLevel())
		PositionOnMultiLevelBackgrounds.Add(MapBackground, -1);
	UpdateBackgroundCache();
}

void UMapViewComponent::UnregisterMultiLevelMapBackground(AMapBackground* MapBackground)
{
	MapBackgrounds.Remove(MapBackground);
	if (MapBackground->IsMultiLevel())
		PositionOnMultiLevelBackgrounds.Remove(MapBackground);
	UpdateBackgroundCache();
}

void UMapViewComponent::UpdateViewSize()
{
	// Precompute view size related values
	const FVector UnscaledBoxExtent = GetUnscaledBoxExtent();
	CachedInverseViewSize.X = 1.0f / (2.0f * UnscaledBoxExtent.X);
	CachedInverseViewSize.Y = 1.0f / (2.0f * UnscaledBoxExtent.Y);
	InverseViewRadius = FMath::Max(CachedInverseViewSize.X, CachedInverseViewSize.Y);

	// Fire view size changed event
	OnViewSizeChanged.Broadcast(this);
}

void UMapViewComponent::UpdateTransformCache()
{
	// See if the component transform has been changed since last call
	const FTransform& Transform = GetComponentTransform();
	if (!Transform.Equals(LastTransform))
	{
		CachedTransform = Transform;

		if (RotationMode == EMapViewRotationMode::UseFixedRotation)
		{
			CachedTransform.SetRotation(FixedRotation.Quaternion());
			//SetWorldRotation(FixedRotation);
		}
		else if (RotationMode == EMapViewRotationMode::InheritYaw)
		{
			const float Yaw = Transform.GetRotation().Rotator().Yaw;
			const FRotator NewRotation = FRotator(0, Yaw + InheritedYawOffset, 0);
			CachedTransform.SetRotation(NewRotation.Quaternion());
			//SetWorldRotation(NewRotation);
		}

		// Precompute inverse from current transform
		CachedInverseTransform = CachedTransform.Inverse();
		
		// Remember which world transform was processed last
		LastTransform = Transform;
	}
}

void UMapViewComponent::UpdateBackgroundCache()
{
	// Refresh background levels periodically
	const float Time = GetWorld()->GetTimeSeconds();
	if (Time - LastBackgroundLevelComputeTime > BackgoundLevelCacheLifetime)
	{
		// Reused variables
		const FVector ViewPos = GetComponentLocation();
		float TempU, TempV;

		// Store the highest priority of surrounding backgrounds
		BackgroundPriority = INT_MIN;
		bInsideAnyBackground = false;
		for (AMapBackground* Background : MapBackgrounds)
		{
			// Skip hidden backgrounds
			if (!Background->IsBackgroundVisible())
				continue;

			// Check if view is inside this background
			if (Background->GetMapView()->GetViewCoordinates(ViewPos, true, TempU, TempV))
			{
				// Store highest priority
				BackgroundPriority = FMath::Max(BackgroundPriority, Background->GetBackgroundPriority());
				bInsideAnyBackground = true;
			}
		}
		
		// For every background, store which level we're on, if inside
		const float Z = HeightProxy ? HeightProxy->GetComponentLocation().Z : ViewPos.Z;
		bInsideMultiLevelBackground = false;
		for (TPair<AMapBackground*, int32>& KVP : PositionOnMultiLevelBackgrounds)
		{
			// If player is outside this multi-level background, mark it
			if (!KVP.Key->GetMapView()->GetViewCoordinates(ViewPos, true, TempU, TempV))
			{
				KVP.Value = INDEX_NONE;
				continue;
			}
			
			// If background is not rendered, behave as if player is outside
			if (!KVP.Key->IsBackgroundVisible() || (bInsideAnyBackground && KVP.Key->GetBackgroundPriority() != BackgroundPriority))
			{
				KVP.Value = INDEX_NONE;
				continue;
			}

			// Player is inside the multi-level background, update the level
			const int32 HeightLevel = KVP.Key->GetLevelAtHeight(Z);
			KVP.Value = HeightLevel;
			bInsideMultiLevelBackground = true;
		}

		// Remember at which time we updated the cache last
		LastBackgroundLevelComputeTime = Time;
	}
}