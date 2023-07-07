// Journeyman's Minimap by ZKShao.

#include "MapFog.h"
#include "MinimapPluginPrivatePCH.h"
#include "MapFunctionLibrary.h"
#include "MapTrackerComponent.h"
#include "MapRevealerComponent.h"
#include "MapViewComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"

AMapFog::AMapFog()
{
	// This actor ticks to update fog
	PrimaryActorTick.bCanEverTick = true;
	
	// Find default fog UMG material in content folder
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultFogMaterial_UMG(TEXT("/MinimapPlugin/Materials/Fog/M_UMG_Fog"));
	FogMaterial_UMG = DefaultFogMaterial_UMG.Object;
	
	// Find default fog Canvas material in content folder
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultFogMaterial_Canvas(TEXT("/MinimapPlugin/Materials/Fog/M_Canvas_Fog"));
	FogMaterial_Canvas = DefaultFogMaterial_Canvas.Object;
	
	// Find default fog combine material in content folder
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultFogCombineMaterial(TEXT("/MinimapPlugin/Materials/Fog/M_FogCombine"));
	FogCombineMaterial = DefaultFogCombineMaterial.Object;
	
	// Find default world fog material in content folder
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultFogPostProcessMaterial(TEXT("/MinimapPlugin/Materials/Fog/M_WorldFog"));
	FogPostProcessMaterial = DefaultFogPostProcessMaterial.Object;
}

void AMapFog::BeginPlay()
{
	Super::BeginPlay();

	// Minimap system is idle on dedicated server but this object is not destroyed,
	// just in case game code references this without checking for dedicated servers.
	if (GetNetMode() == ENetMode::NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
		return;
	}

	// Possibly set up world fog
	InitializeWorldFog();
	
	// Create dynamic render targets to hold permanent and temporary revealed locations
	const int32 RenderTargetSize = FMath::Max(2, FogRenderTargetSize);
	PermanentRevealRT_A = UKismetRenderingLibrary::CreateRenderTarget2D(this, RenderTargetSize, RenderTargetSize);
	PermanentRevealRT_B = UKismetRenderingLibrary::CreateRenderTarget2D(this, RenderTargetSize, RenderTargetSize);
	RevealRT_Staging = UKismetRenderingLibrary::CreateRenderTarget2D(this, RenderTargetSize, RenderTargetSize);
	
	// Register self to tracker
	UMapTrackerComponent* Tracker = UMapFunctionLibrary::GetMapTracker(this);
	if (Tracker)
	{
		Tracker->RegisterMapFog(this);
		Tracker->OnMapRevealerRegistered.AddUniqueDynamic(this, &AMapFog::OnMapRevealerRegistered);
		Tracker->OnMapRevealerUnregistered.AddUniqueDynamic(this, &AMapFog::OnMapRevealerUnregistered);
		
		// Register initial revealers
		TArray<UMapRevealerComponent*> Revealers = Tracker->GetMapRevealers();
		for (UMapRevealerComponent* Revealer : Revealers)
			OnMapRevealerRegistered(Revealer);
	}
	
	// Initialize animation start time
	AnimStartTime = GetWorld()->GetTimeSeconds();

	if (FogCombineMaterial)
	{
		FogCombineMatInst = UMaterialInstanceDynamic::Create(FogCombineMaterial, this);
		FogCombineMatInst->SetTextureParameterValue(TEXT("NewFog"), RevealRT_Staging);
	}
}

void AMapFog::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (GetNetMode() == ENetMode::NM_DedicatedServer)
		return;

	// Unregister self from tracker
	UMapTrackerComponent* Tracker = UMapFunctionLibrary::GetMapTracker(this);
	if (Tracker)
	{
		Tracker->UnregisterMapFog(this);
		Tracker->OnMapRevealerRegistered.RemoveDynamic(this, &AMapFog::OnMapRevealerRegistered);
		Tracker->OnMapRevealerUnregistered.RemoveDynamic(this, &AMapFog::OnMapRevealerUnregistered);
	}
}

void AMapFog::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Clear the temporary vision render target
	UKismetRenderingLibrary::ClearRenderTarget2D(this, RevealRT_Staging, FLinearColor::Black);
	
	// Start rendering to the 'staging' fog render target, which will hold this frame's newly revealed locations
	UCanvas* Canvas;
	FVector2D Size;
	FDrawToRenderTargetContext RenderContext;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RevealRT_Staging, Canvas, Size, RenderContext);
	
	// Categorize all revealers
	TSet<UMapRevealerComponent*> TemporaryRevealers;
	TSet<UMapRevealerComponent*> PermanentRevealers;
	for (UMapRevealerComponent* Revealer : MapRevealers)
	{
		if (Revealer->GetRevealMode() == EMapFogRevealMode::Off)
			continue;
		Revealer->UpdateMapFog(this, Canvas);
	}

	// Finish rendering to the staging fog render target
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, RenderContext);

	// Combine the newly revealed locations with already revealed locations. Last frame's and this frame's 
	// permanently revealed areas are combined. Last frame's temporary revealed area is ignored.
	if (FogCombineMatInst)
	{
		// We swap between two buffers: One buffer has the old data, the other 
		// will contain the new data. Their roles change every frame.
		UTextureRenderTarget2D* OldRT = bUseBufferA ? PermanentRevealRT_A : PermanentRevealRT_B;
		UTextureRenderTarget2D* NewRT = bUseBufferA ? PermanentRevealRT_B : PermanentRevealRT_A;
		bUseBufferA = !bUseBufferA;

		// Update the combine material's old buffer reference, render to the new buffer
		FogCombineMatInst->SetTextureParameterValue(TEXT("OldFog"), OldRT);
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, NewRT, FogCombineMatInst);
		
		// If using a fog post process effect, update the active buffer reference
		if (FogPostProcessMatInst)
			FogPostProcessMatInst->SetTextureParameterValue(TEXT("FogRenderTarget"), GetDestinationFogRenderTarget());
	}

	// Mark render target contents retrieved from GPU as dirty
	const float Time = GetWorld()->GetTimeSeconds();
	if (Time - StagingRT_LastReadTime > FogCacheLifetime)
		bStagingRT_Read = false;
	if (Time - PermanentRT_LastReadTime > FogCacheLifetime)
		bPermanentRT_Read = false;
}

bool AMapFog::GetFogAtLocation(const FVector& WorldLocation, const bool bRequireCurrentlyRevealing, float& RevealFactor)
{
	float U, V;
	if (FogRenderTargetSize <= 0 || !GetMapView()->GetViewCoordinates(WorldLocation, false, U, V))
		return false;
	
	// Read fog render target contents from the GPU, this is done at max once per frame
	bool& bRelevantReadFlag = bRequireCurrentlyRevealing ? bStagingRT_Read : bPermanentRT_Read;
	TArray<FLinearColor>& RelevantBuffer = bRequireCurrentlyRevealing ? StagingRT_Buffer : PermanentRT_Buffer;
	float& RelevantLastReadTime = bRequireCurrentlyRevealing ? StagingRT_LastReadTime : PermanentRT_LastReadTime;
	if (!bRelevantReadFlag)
	{
		// Read the render target into the buffer
		UTextureRenderTarget2D* RelevantRenderTarget = bRequireCurrentlyRevealing ? RevealRT_Staging : PermanentRevealRT_A;
		FTextureRenderTarget2DResource* TextureResource = static_cast<FTextureRenderTarget2DResource*>(RelevantRenderTarget->GetResource());
		TextureResource->ReadLinearColorPixels(RelevantBuffer);
		checkf(RelevantBuffer.Num() > 0, TEXT("Expected pixels to be retrieved"));
		
		// Remember that this buffer is read for this frame
		bRelevantReadFlag = true;
		RelevantLastReadTime = GetWorld()->GetTimeSeconds();
	}

	// Convert the view coordinates to a 1D index
	const int32 i = FMath::RoundToInt(U * FogRenderTargetSize);
	const int32 j = FMath::RoundToInt(V * FogRenderTargetSize);
	const int32 PixelIndex = FMath::Clamp(j * FogRenderTargetSize + i, 0, RelevantBuffer.Num() - 1);

	// R-channel = permanently revealed. G-channel = temporarily revealed.
	const FLinearColor PixelValue = RelevantBuffer[PixelIndex];
	RevealFactor = FMath::Clamp(FMath::Max(PixelValue.R, PixelValue.G), 0.0f, 1.0f);
	return true;
}

UTextureRenderTarget2D* AMapFog::GetDestinationFogRenderTarget() const
{
	return bUseBufferA ? PermanentRevealRT_B : PermanentRevealRT_A;
}

UTextureRenderTarget2D* AMapFog::GetSourceFogRenderTarget() const
{
	return bUseBufferA ? PermanentRevealRT_A : PermanentRevealRT_B;
}

float AMapFog::GetWorldToPixelRatio() const
{
	const float WorldSize = 2.0f * GetAreaBounds()->GetScaledBoxExtent().X;
	return (WorldSize > 0) ? (static_cast<float>(FogRenderTargetSize) / WorldSize) : 1.0f;
}

void AMapFog::SetFogMaterialForUMG(UMaterialInterface* NewMaterial)
{
	FogMaterial_UMG = NewMaterial;
	OnMapFogMaterialChanged.Broadcast(this);
}

UMaterialInterface* AMapFog::GetFogMaterialForUMG()
{
	return FogMaterial_UMG;
}

void AMapFog::SetFogMaterialForCanvas(UMaterialInterface* NewMaterial)
{
	// Change the material and fire event
	FogMaterial_Canvas = NewMaterial;
	OnMapFogMaterialChanged.Broadcast(this);

	// Clear all existing material instances so that they will be re-instanced
	MaterialInstances.Empty();
	
	// Refresh animation start time
	AnimStartTime = GetWorld()->GetTimeSeconds();
}

UMaterialInstanceDynamic* AMapFog::GetFogMaterialInstanceForCanvas(UMapRendererComponent* Renderer)
{
	if (!Renderer || !FogMaterial_Canvas)
		return nullptr;

	// See if a material instance has already been created for this renderer
	if (!MaterialInstances.Contains(Renderer))
	{
		// Create a new material instance for this renderer
		UMaterialInstanceDynamic* NewInst = UMaterialInstanceDynamic::Create(FogMaterial_Canvas, this);
		MaterialInstances.Add(Renderer, NewInst);
		NewInst->SetScalarParameterValue(TEXT("OpacityHidden"), MinimapOpacityHidden);
		NewInst->SetScalarParameterValue(TEXT("OpacityExplored"), MinimapOpacityExplored);
		NewInst->SetScalarParameterValue(TEXT("OpacityViewing"), MinimapOpacityRevealing);
	}

	// Return the existing material instance after updating the material instance's Time parameter for animations
	UMaterialInstanceDynamic* MatInst = MaterialInstances[Renderer];
	MatInst->SetScalarParameterValue(TEXT("Time"), GetWorld()->GetTimeSeconds() - AnimStartTime);
	MatInst->SetTextureParameterValue(TEXT("FogRenderTarget"), GetDestinationFogRenderTarget());
	return MatInst;
}

void AMapFog::InitializeWorldFog()
{
	// If no fog post process material is set, abort
	if (!bEnableWorldFog || !FogPostProcessMaterial)
		return;
	
	// Unwrap auto settings
	bool bAllowAutoLocate = false;
	bool bAllowAutoCreate = false;
	switch (AutoLocatePostProcessVolume)
	{
	case EFogPostProcessVolumeOption::AutoLocate:
		bAllowAutoLocate = true;
		break;
	case EFogPostProcessVolumeOption::AutoLocateOrCreate:
		bAllowAutoLocate = true;
		bAllowAutoCreate = true;
		break;
	default: ;
	}

	// If auto-locate post process volume is enabled, look for it now
	if (!PostProcessVolume && bAllowAutoLocate)
	{
		// Give priority to any unbound volume
		for (TActorIterator<APostProcessVolume> PPItr(GetWorld()); PPItr && !PostProcessVolume; ++PPItr)
			if (PPItr->bUnbound)
				PostProcessVolume = *PPItr;
		
		// If no unbound volume was found, then use any volume
		for (TActorIterator<APostProcessVolume> PPItr(GetWorld()); PPItr && !PostProcessVolume; ++PPItr)
			PostProcessVolume = *PPItr;
	}
	
	// If auto-create post process volume is enabled, create it now
	if (!PostProcessVolume && bAllowAutoCreate)
	{
		PostProcessVolume = GetWorld()->SpawnActor<APostProcessVolume>();
		PostProcessVolume->bUnbound = true;
	}

	// Abort if no post process volume was set, nor found, nor created
	if (!PostProcessVolume)
		return;

	// Initialize the post process effect
	FogPostProcessMatInst = UMaterialInstanceDynamic::Create(FogPostProcessMaterial, this);

	// Pass reference to this fog's render target
	FogPostProcessMatInst->SetTextureParameterValue(TEXT("FogRenderTarget"), GetDestinationFogRenderTarget());

	// Pass this fog's location
	const FVector FogLocation = GetActorLocation();
	const FVector FogExtent = GetAreaBounds()->GetScaledBoxExtent();
	const float FogAngle = GetActorRotation().Yaw;
	FLinearColor FogVolumeBounds(FogLocation.X, FogLocation.Y, FogExtent.X, FogExtent.Y);
	FogPostProcessMatInst->SetVectorParameterValue(TEXT("FogVolumeBounds"), FogVolumeBounds);
	FogPostProcessMatInst->SetScalarParameterValue(TEXT("FogVolumeAngle"), FogAngle);
	FogPostProcessMatInst->SetScalarParameterValue(TEXT("OpacityHidden"), WorldOpacityHidden);
	FogPostProcessMatInst->SetScalarParameterValue(TEXT("OpacityExplored"), WorldOpacityExplored);
	FogPostProcessMatInst->SetScalarParameterValue(TEXT("OpacityViewing"), WorldOpacityRevealing);

	// Add the material instance to the post process volume's blendables
	PostProcessVolume->AddOrUpdateBlendable(FogPostProcessMatInst);
}

void AMapFog::OnMapRevealerRegistered(UMapRevealerComponent* MapRevealer)
{
	MapRevealers.Add(MapRevealer);
}

void AMapFog::OnMapRevealerUnregistered(UMapRevealerComponent* MapRevealer)
{
	MapRevealers.RemoveSingle(MapRevealer);
}