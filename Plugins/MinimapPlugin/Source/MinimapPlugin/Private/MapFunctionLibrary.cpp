// Journeyman's Minimap by ZKShao.

#include "MapFunctionLibrary.h"
#include "MinimapPluginPrivatePCH.h"
#include "MapTrackerComponent.h"
#include "MapViewComponent.h"
#include "MapIconComponent.h"
#include "MapBackground.h"
#include "MapFog.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameState.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"

UMapTrackerComponent* UMapFunctionLibrary::GetMapTracker(const UObject* WorldContextObject)
{
	// Retrieve the world from the context object
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 16
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
#else
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
#endif
	if (!World)
		return nullptr;

	// Minimap is pointless on dedicated server, so don't do any tracking
	if (World->GetNetMode() == ENetMode::NM_DedicatedServer)
		return nullptr;

	// Retrieve the game state actor
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 13
	AGameState* GS = World->GetGameState();
#else
	AGameStateBase* GS = World->GetGameState();
#endif

	// Game state not found, assuming gameplay is not in progress
	if (!GS)
		return nullptr;

	// Attempt to find a MapTrackerComponent as component of the GameState actor
	UMapTrackerComponent* MapTracker = Cast<UMapTrackerComponent>(GS->GetComponentByClass(UMapTrackerComponent::StaticClass()));
	if (MapTracker)
	{
		// Found it, so return it
		return MapTracker;
	}
	else
	{
		// Wasn't found, so create it once. Subsequent calls will find this one.
		MapTracker = NewObject<UMapTrackerComponent>(GS, TEXT("MapTracker"));
		return MapTracker;
	}
}

AMapBackground* UMapFunctionLibrary::GetFirstMapBackground(const UObject* WorldContextObject)
{
	// Find a MapBackground in the level and return it, if there is any
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 16
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
#else
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
#endif
	for (TActorIterator<AMapBackground> Itr(World); Itr; ++Itr)
		return *Itr;

	// None found
	return nullptr;
}

UMapViewComponent* UMapFunctionLibrary::FindMapView(UObject* WorldContextObject, const EMapViewSearchOption MapViewSearchOption)
{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 16
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
#else
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
#endif

	bool bConsiderPlayer = false;
	bool bConsiderMapBackground = false;
	bool bConsiderMapFog = false;
	bool bConsiderAllActors = false;
	switch (MapViewSearchOption)
	{
	case EMapViewSearchOption::Any:
		bConsiderPlayer = true;
		bConsiderMapBackground = true;
		bConsiderMapFog = true;
		bConsiderAllActors = true;
		break;
	case EMapViewSearchOption::OnPlayer:
		bConsiderPlayer = true;
		break;
	case EMapViewSearchOption::OnMapBackground:
		bConsiderMapBackground = true;
		break;
	case EMapViewSearchOption::OnMapFog:
		bConsiderMapFog = true;
		break;
	case EMapViewSearchOption::Disabled:
		return nullptr;
	}

	if (bConsiderPlayer)
	{
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(WorldContextObject, 0);
		UMapViewComponent* MapView = PlayerPawn ? PlayerPawn->FindComponentByClass<UMapViewComponent>() : nullptr;
		if (MapView)
			return MapView;

		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
		MapView = PlayerController ? PlayerController->FindComponentByClass<UMapViewComponent>() : nullptr;
		if (MapView)
			return MapView;
	}

	if (bConsiderMapBackground)
		for (TActorIterator<AMapBackground> Itr(World); Itr; ++Itr)
			return Itr->GetMapView();
	
	if (bConsiderMapFog)
		for (TActorIterator<AMapFog> Itr(World); Itr; ++Itr)
			return Itr->GetMapView();

	if (bConsiderAllActors)
		for (TObjectIterator<UMapViewComponent> Itr; Itr; ++Itr)
			if (Itr->GetWorld() == World)
				return *Itr;

	return nullptr;
}

bool UMapFunctionLibrary::DetectIsInView(const FVector2D& UV, const FVector2D& OuterRadiusUV, const bool bIsCircular)
{
	if (bIsCircular)
	{
		// Return true if any part of the icon is in the circular view
		return FMath::Pow(UV.X - 0.5f, 2) + FMath::Pow(UV.Y - 0.5f, 2) < FMath::Pow(0.5f + OuterRadiusUV.X, 2);
	}
	else
	{
		// Return true if any part of the icon is in the rectangular view
		return UV.X > -OuterRadiusUV.X && UV.X < 1 + OuterRadiusUV.X && UV.Y > -OuterRadiusUV.Y && UV.Y < 1 + OuterRadiusUV.Y;
	}
}

FVector2D UMapFunctionLibrary::ClampIntoView(const FVector2D& UV, const float OuterRadiusUV, const bool bIsCircular)
{
	FVector2D UVOut;
	if (bIsCircular)
	{
		// Move the icon completely into the circular view
		const float Angle = FMath::Atan2(UV.Y - 0.5f, UV.X - 0.5f);
		const float Radius = 0.5f - OuterRadiusUV;
		UVOut.X = 0.5f + FMath::Cos(Angle) * Radius;
		UVOut.Y = 0.5f + FMath::Sin(Angle) * Radius;
	}
	else
	{
		// Move the icon completely into the rectangular view, but make sure its still in the same direction
		const float CenteredX = UV.X - 0.5f;
		const float CenteredY = UV.Y - 0.5f;
		const float Angle = FMath::Atan2(CenteredY, CenteredX);
		if (FMath::Abs(CenteredX) > FMath::Abs(CenteredY))
		{
			// Move to X-boundary and set Y-coordinate so icon stays in the same direction
			const float ClampedX = FMath::Sign(CenteredX) * (0.5f - OuterRadiusUV);
			UVOut.X = 0.5f + ClampedX;
			UVOut.Y = 0.5f + FMath::Tan(Angle) * ClampedX;
		}
		else
		{
			// Move to Y-boundary and set X-coordinate so icon stays in the same direction
			const float ClampedY = FMath::Sign(CenteredY) * (0.5f - OuterRadiusUV);
			UVOut.X = 0.5f + ClampedY / FMath::Tan(Angle);
			UVOut.Y = 0.5f + ClampedY;
		}
	}

	return UVOut;
}

TArray<UMapIconComponent*> UMapFunctionLibrary::BoxSelectInView(const FVector2D& StartUV, const FVector2D& EndUV, UMapViewComponent* MapView, const bool bIsCircular)
{
	UMapTrackerComponent* MapTracker = UMapFunctionLibrary::GetMapTracker(MapView);
	if (!MapTracker)
		return TArray<UMapIconComponent*>();
	
	const FVector2D UVMin(FMath::Min(StartUV.X, EndUV.X), FMath::Min(StartUV.Y, EndUV.Y));
	const FVector2D UVMax(FMath::Max(StartUV.X, EndUV.X), FMath::Max(StartUV.Y, EndUV.Y));

	float U, V;
	TArray<UMapIconComponent*> Results;
	TArray<UMapIconComponent*> AllIcons = MapTracker->GetMapIcons();
	for (UMapIconComponent* MapIcon : AllIcons)
	{
		// Check if icon is not hidden
		if (!MapIcon->IsIconVisible())
			continue;

		// Check if icon is not hidden for this view, for example its in a background priority volume that the map view cannot see at the moment
		if (!MapIcon->IsRenderedInView(MapView))
			continue;

		// Check if icon is in view rect, then if its in minimap shape, then finally if its in the box
		if (MapView->GetViewCoordinates(MapIcon->GetComponentLocation(), bIsCircular, U, V) && DetectIsInView(FVector2D(U, V), FVector2D::ZeroVector, bIsCircular) && U >= UVMin.X && U <= UVMax.X && V >= UVMin.Y && V <= UVMax.Y)
			Results.Add(MapIcon);
	}

	return Results;
}

bool UMapFunctionLibrary::ComputeViewFrustum(const UObject* WorldContextObject, UMapViewComponent* MapView, const bool bIsCircular, TArray<FVector2D>& CornerUVs, float FloorDistance /* = 600.0f*/)
{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 16
    const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
#else
    const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
#endif
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
		return false;

	// Compute player view pitch
	FVector ViewPos;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewPos, ViewRot);
	const FVector ViewDir = ViewRot.Vector();
	while (ViewRot.Pitch > 180)
		ViewRot.Pitch -= 360;
	
	// Abort if player is looking towards the horizon
	if (ViewRot.Pitch >= -40.0f)
		return false;

	// Get viewport corners
	int32 Width, Height;
	PC->GetViewportSize(Width, Height);
	TArray<FVector2D> ViewportCorners2D;
	ViewportCorners2D.Add(FVector2D(0, 0));
	ViewportCorners2D.Add(FVector2D(Width, 0));
	ViewportCorners2D.Add(FVector2D(Width, Height));
	ViewportCorners2D.Add(FVector2D(0, Height));
	
	// Define a virtual floor plane below the camera
	const float FloorZ = ViewPos.Z - FloorDistance;
	const FPlane FloorPlane(FVector::UpVector, FloorZ);

	// Generate rays through the viewport corners and compute their intersection with the virtual floor plane
	TArray<FVector> WorldCorners;
	for (const auto& P : ViewportCorners2D)
	{
		// Deproject and apply pitch offset
		FVector WorldPos, WorldDir;
		UGameplayStatics::DeprojectScreenToWorld(PC, P, WorldPos, WorldDir);

		// Abort if the ray through the screen corner goes up
		if (WorldDir.Z >= 0)
			return false;

		// Intersect with floor plane
		float T; FVector FloorPos;
		if (!UKismetMathLibrary::LinePlaneIntersection(WorldPos, WorldPos + 100000.0f * WorldDir, FloorPlane, T, FloorPos))
			return false;

		// Add point
		WorldCorners.Add(FloorPos);
	}

	// Convert the floor projected corners to minimap space
	for (uint8 i = 0; i < 4; ++i)
	{
		float U, V;
		MapView->GetViewCoordinates(WorldCorners[i], bIsCircular, U, V);
		CornerUVs.Add(FVector2D(U, V));
	}
	return true;
}
