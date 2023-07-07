// Journeyman's Minimap by ZKShao.

#include "MapAreaBase.h"
#include "MinimapPluginPrivatePCH.h"
#include "MapViewComponent.h"
#include "Components/BoxComponent.h"

FBoxSphereBounds UMapAreaPrimitiveComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const float LargestExtent = FMath::Max(FMath::Max(ScaledBoxExtent.X, ScaledBoxExtent.Y), ScaledBoxExtent.Z);
	return FBoxSphereBounds(LocalToWorld.GetLocation(), ScaledBoxExtent, FMath::Sqrt(2.0f) * LargestExtent);
}

AMapAreaBase::AMapAreaBase()
{
	// Create box volume that represents the part of the world that is covered by this map
	AreaBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("AreaBounds"));
	AreaBounds->SetBoxExtent(FVector(2048, 2048, 1024), true);
	AreaBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(AreaBounds);

	// Make a sprite to make this actor easier to spot in the level viewport
	AreaPrimitive = CreateDefaultSubobject<UMapAreaPrimitiveComponent>(TEXT("AreaPrimitive"));
	AreaPrimitive->SetupAttachment(AreaBounds);
	AreaPrimitive->ScaledBoxExtent = AreaBounds->GetScaledBoxExtent();
	
	// Create full map view component, this can be used to render a minimap that covers exactly the area of MapWorldBounds
	AreaMapView = CreateDefaultSubobject<UMapViewComponent>(TEXT("AreaMapView"));
	AreaMapView->SetupAttachment(AreaBounds);
	AreaMapView->RotationMode = EMapViewRotationMode::UseFixedRotation;
	AreaMapView->FixedRotation = FRotator::ZeroRotator;
	AreaMapView->InheritedYawOffset = 0.0f;
	AreaMapView->bSupportZooming = false;
	AreaMapView->SetVisibility(false);
	AreaMapView->bSelectable = false;
}

void AMapAreaBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyAreaBounds();
}

void AMapAreaBase::BeginPlay()
{
	Super::BeginPlay();
	ApplyAreaBounds();
}

#if WITH_EDITOR
void AMapAreaBase::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	ApplyAreaBounds();
}
#endif

UBoxComponent* AMapAreaBase::GetAreaBounds() const
{
	return AreaBounds;
}

UMapViewComponent* AMapAreaBase::GetMapView() const
{
	return AreaMapView;
}

float AMapAreaBase::GetMapAspectRatio() const
{
	// Returns the aspect ratio of the area covered by this area
	const FVector ScaledBoxExtent = GetAreaBounds()->GetScaledBoxExtent();
	return (ScaledBoxExtent.Y != 0) ? ScaledBoxExtent.X / ScaledBoxExtent.Y : 1.0f;
}

bool AMapAreaBase::GetMapViewCornerUVs(UMapViewComponent* MapView, TArray<FVector2D>& CornerUVs)
{
	CornerUVs.Empty();
	CornerUVs.SetNumZeroed(4);
	if (MapView == AreaMapView)
	{
		// Detect special case when we're actually rendering the volume's view itself, to save some computations
		CornerUVs[0] = FVector2D(0, 0);
		CornerUVs[1] = FVector2D(1, 0);
		CornerUVs[2] = FVector2D(1, 1);
		CornerUVs[3] = FVector2D(0, 1);
			
		// Crop UVs because render target and map volume may have different aspect ratios
		const int32 Level = GetLevelAtHeight(MapView->GetComponentLocation().Z);
		for (uint8 i = 0; i < 4; ++i)
			CornerUVs[i] = CorrectUVs(Level, CornerUVs[i]);
		return true;
	}
	else
	{
		// Compute what section of the background to render
		const TArray<FVector> Corners = MapView->GetWorldCorners();
		const int32 Level = GetLevelAtHeight(MapView->GetComponentLocation().Z);
		for (uint8 i = 0; i < 4; ++i)
		{
			// Compute full map UV coordinates of world corners
			float CornerX, CornerY;
			AreaMapView->GetViewCoordinates(Corners[i], false, CornerX, CornerY);
			CornerUVs[i].X = CornerX;
			CornerUVs[i].Y = CornerY;

			// Crop UVs because auto-generated background is always square whereas the map volume may not be
			CornerUVs[i] = CorrectUVs(Level, CornerUVs[i]);
		}
				
		// Construct axis aligned bounding box
		FVector2D UVMin = CornerUVs[0];
		FVector2D UVMax = CornerUVs[0];
		for (uint8 i = 1; i < 4; ++i)
		{
			UVMin.X = FMath::Min(UVMin.X, CornerUVs[i].X);
			UVMin.Y = FMath::Min(UVMin.Y, CornerUVs[i].Y);
			UVMax.X = FMath::Max(UVMax.X, CornerUVs[i].X);
			UVMax.Y = FMath::Max(UVMax.Y, CornerUVs[i].Y);
		}

		// Check whether the AABB intersects the background
		return !(UVMax.X < 0 || UVMin.X > 1 || UVMax.Y < 0 || UVMin.Y > 1);
	}
}

int32 AMapAreaBase::GetLevelAtHeight(const float WorldZ) const
{
	return 0;
}

FVector2D AMapAreaBase::CorrectUVs(const int32 Level, const FVector2D& InUV) const
{
	// Can be overridden to perform transformations to UVs
	return InUV;
}

void AMapAreaBase::ApplyAreaBounds()
{
	// Update map view
	const FVector ScaledBoxExtent = AreaBounds->GetScaledBoxExtent();
	AreaMapView->SetViewExtent(ScaledBoxExtent.X, ScaledBoxExtent.Y);

	// Update selection primitive
	AreaPrimitive->SetRelativeLocation(FVector::ZeroVector);
	AreaPrimitive->ScaledBoxExtent = ScaledBoxExtent;

	// 
	AreaMapView->RotationMode = EMapViewRotationMode::UseFixedRotation;
	AreaMapView->FixedRotation = GetActorRotation();
}
