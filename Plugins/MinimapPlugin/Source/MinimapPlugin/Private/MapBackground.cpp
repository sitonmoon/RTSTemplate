// Journeyman's Minimap by ZKShao.

#include "MapBackground.h"
#include "MinimapPluginPrivatePCH.h"
#include "MapTrackerComponent.h"
#include "MapViewComponent.h"
#include "MapFunctionLibrary.h"

#include "NavMesh/NavMeshRenderingComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

AMapBackground::AMapBackground()
{
	// Create capture component for generating top-down snapshot
	CaptureComponent2D = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComponent2D"));
	CaptureComponent2D->SetupAttachment(GetRootComponent());
	
	// Point camera downward, set to orthographic, low dynamic range and don't continuously render
	CaptureComponent2D->SetWorldRotation(FRotator(-90, -90, 0));
	CaptureComponent2D->ProjectionType = ECameraProjectionMode::Type::Orthographic;
	CaptureComponent2D->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	CaptureComponent2D->bCaptureEveryFrame = false;

	// Ideally fog doesn't show up in scene capture. Unfortunately, fog still shows up. 
	// This is a known engine bug: https://issues.unrealengine.com/issue/UE-35666
	// Still setting the flag to false in case this might be addressed later.
	CaptureComponent2D->ShowFlags.SetFog(false);

	// Prepares rendering navigation mesh to generated background
	CaptureComponent2D->ShowFlags.SetNavigation(true);
	NavMeshRenderingComponent = CreateDefaultSubobject<UNavMeshRenderingComponent>(TEXT("NavMeshRenderer"));
	NavMeshRenderingComponent->SetupAttachment(GetRootComponent());
	NavMeshRenderingComponent->SetHiddenInGame(true);
	NavMeshRenderingComponent->bIsEditorOnly = false;
	
	// Loads default material for rendering the background in UMG
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder_UMG(TEXT("/MinimapPlugin/Materials/Background/M_UMG_Background"));
	BackgroundMaterial_UMG = MaterialFinder_UMG.Object;
	
	// Loads default material for rendering the background on UCanvas
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder_Canvas(TEXT("/MinimapPlugin/Materials/Background/M_Canvas_Background"));
	BackgroundMaterial_Canvas = MaterialFinder_Canvas.Object;
	
	// Loads default render target asset to store generated snapshot in
	static ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> RenderTargetAssetFinder(TEXT("/MinimapPlugin/Textures/RT_MinimapSnapshot"));

	FMapBackgroundLevel DefaultBackgroundLevel;
	DefaultBackgroundLevel.RenderTarget = RenderTargetAssetFinder.Object;
	BackgroundLevels.Empty();
	BackgroundLevels.Add(DefaultBackgroundLevel);

	// By default, hide pawns from dynamically rendered snapshots
	HiddenActorClasses.Add(APawn::StaticClass());
}

#if WITH_EDITOR
void AMapBackground::PostLoad()
{
	Super::PostLoad();
	NormalizeScale();
	VisualizeLevelsInEditor();
}

void AMapBackground::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	NormalizeScale();
	VisualizeLevelsInEditor();
}

void AMapBackground::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	NormalizeScale();
	ApplyBackgroundTexture();
	VisualizeLevelsInEditor();
}
#endif

void AMapBackground::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITOR
	VisualizeLevelsInEditor();
#endif
}

void AMapBackground::BeginPlay()
{
	Super::BeginPlay();
	
	// Minimap system is idle on dedicated server but this object is not destroyed,
	// just in case game code references this without checking for dedicated servers.
	if (GetNetMode() == ENetMode::NM_DedicatedServer)
		return;

	// Ensure at least one background level exists
	if (BackgroundLevels.Num() == 0)
		BackgroundLevels.Add(FMapBackgroundLevel());

	// Create dynamic render targets for background levels that don't have a texture or static render target set
	InitializeDynamicRenderTargets();

	// Precompute some background related values and take a snapshot if required
	ApplyBackgroundTexture();
	
	// Register self to tracker
	UMapTrackerComponent* Tracker = UMapFunctionLibrary::GetMapTracker(this);
	if (Tracker)
		Tracker->RegisterMapBackground(this);

	// Initialize animation start time
	AnimStartTime = GetWorld()->GetTimeSeconds();
}

void AMapBackground::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (GetNetMode() == ENetMode::NM_DedicatedServer)
		return;
	
	// Unregister self from tracker
	UMapTrackerComponent* Tracker = UMapFunctionLibrary::GetMapTracker(this);
	if (Tracker)
		Tracker->UnregisterMapBackground(this);
}

void AMapBackground::SetBackgroundMaterialForUMG(UMaterialInterface* NewMaterial)
{
	// Change the material and fire event
	BackgroundMaterial_UMG = NewMaterial;
	OnMapBackgroundMaterialChanged.Broadcast(this);
	OnMapBackgroundAppearanceChanged.Broadcast(this);
}

UMaterialInterface* AMapBackground::GetBackgroundMaterialForUMG() const
{
	// Returns the base material to render the background with in UMG
	return BackgroundMaterial_UMG;
}

void AMapBackground::SetBackgroundMaterialForCanvas(UMaterialInterface* NewMaterial)
{
	// Change the material and fire event
	BackgroundMaterial_Canvas = NewMaterial;
	OnMapBackgroundMaterialChanged.Broadcast(this);
	OnMapBackgroundAppearanceChanged.Broadcast(this);

	// Clear all existing material instances so that they will be re-instanced
	MaterialInstances.Empty();
	
	// Refresh animation start time
	AnimStartTime = GetWorld()->GetTimeSeconds();
}

UMaterialInstanceDynamic* AMapBackground::GetBackgroundMaterialInstanceForCanvas(UMapRendererComponent* Renderer)
{
	if (!Renderer || !BackgroundMaterial_Canvas)
		return nullptr;

	// See if a material instance has already been created for this renderer
	if (!MaterialInstances.Contains(Renderer))
	{
		// Create a new material instance for this renderer
		UMaterialInstanceDynamic* NewInst = UMaterialInstanceDynamic::Create(BackgroundMaterial_Canvas, this);
		NewInst->SetTextureParameterValue(TEXT("Texture"), GetBackgroundTexture());
		MaterialInstances.Add(Renderer, NewInst);
	}

	// Return the existing material instance after updating the material instance's Time parameter for animations
	UMaterialInstanceDynamic* MatInst = MaterialInstances[Renderer];
	MatInst->SetScalarParameterValue(TEXT("Time"), GetWorld()->GetTimeSeconds() - AnimStartTime);
	return MatInst;
}

void AMapBackground::SetBackgroundVisible(const bool bNewVisible)
{
	bBackgroundVisible = bNewVisible;
	OnMapBackgroundAppearanceChanged.Broadcast(this);
}

bool AMapBackground::IsBackgroundVisible() const
{
	return bBackgroundVisible;
}

void AMapBackground::SetBackgroundPriority(const int32 NewBackgroundPriority)
{
	BackgroundPriority = NewBackgroundPriority;
	OnMapBackgroundAppearanceChanged.Broadcast(this);
}

int32 AMapBackground::GetBackgroundPriority() const
{
	return BackgroundPriority;
}

void AMapBackground::SetBackgroundZOrder(const int32 NewBackgroundZOrder)
{
	BackgroundZOrder = NewBackgroundZOrder;
	OnMapBackgroundAppearanceChanged.Broadcast(this);
}

int32 AMapBackground::GetBackgroundZOrder() const
{
	return BackgroundZOrder;
}

bool AMapBackground::IsMultiLevel() const
{
	return BackgroundLevels.Num() > 1;
}

void AMapBackground::SetBackgroundTexture(const int32 Level, UTexture2D* NewBackgroundTexture)
{
	if (BackgroundLevels.IsValidIndex(Level) && NewBackgroundTexture != BackgroundLevels[Level].BackgroundTexture)
	{
		BackgroundLevels[Level].BackgroundTexture = NewBackgroundTexture;
		ApplyBackgroundTexture();
	}
}

UTexture* AMapBackground::GetBackgroundTexture(const int32 Level) const
{
	if (BackgroundLevels.Num() == 0)
		return nullptr;
	
	// If a background texture is set, return it
	// Otherwise return the render target in which the snapshot is stored
	const int32 Index = FMath::Clamp(Level, 0, BackgroundLevels.Num() - 1);
	if (BackgroundLevels[Index].BackgroundTexture)
		return BackgroundLevels[Index].BackgroundTexture;
	else
		return BackgroundLevels[Index].RenderTarget;
}

void AMapBackground::SetBackgroundOverlay(const int32 Level /*= 0*/, UTextureRenderTarget2D* NewOverlay /*= nullptr*/)
{
	if (BackgroundLevels.IsValidIndex(Level) && NewOverlay != BackgroundLevels[Level].Overlay)
	{
		BackgroundLevels[Level].Overlay = NewOverlay;
		OnMapBackgroundOverlayChanged.Broadcast(this, Level, NewOverlay);
	}
}

UTextureRenderTarget2D* AMapBackground::GetBackgroundOverlay(const int32 Level /*= 0*/) const
{
	return BackgroundLevels.IsValidIndex(Level) ? BackgroundLevels[Level].Overlay : nullptr;
}

int32 AMapBackground::GetLevelAtHeight(const float WorldZ) const
{
	if (BackgroundLevels.Num() == 0)
		return INDEX_NONE;

	const float ScaledBoxExtentZ = GetAreaBounds()->GetScaledBoxExtent().Z;
	const float MinZ = GetAreaBounds()->GetComponentLocation().Z - ScaledBoxExtentZ;
	const float RelativeZ = WorldZ - MinZ;
	
	int32 LevelIndex = 0;
	float LevelCeiling = 0;
	for (int32 i = 0; i < BackgroundLevels.Num() && RelativeZ > LevelCeiling; ++i)
	{
		// Reached this level, but can possible continue
		LevelIndex = i;

		// Update ceiling. Last level uses remainder
		LevelCeiling += BackgroundLevels[i].LevelHeight;
	}

	return LevelIndex;
}

UTexture* AMapBackground::GetBackgroundTextureAtHeight(const float WorldZ) const
{
	const int32 LevelIndex = GetLevelAtHeight(WorldZ);
	if (BackgroundLevels.IsValidIndex(LevelIndex))
	{
		if (BackgroundLevels[LevelIndex].BackgroundTexture)
			return BackgroundLevels[LevelIndex].BackgroundTexture;
		else
			return BackgroundLevels[LevelIndex].RenderTarget;
	}
	return nullptr;
}

void AMapBackground::RerenderBackground()
{
	// Render a new background snapshot for background levels without static texture assigned
	ApplyBackgroundTexture();
}

FVector2D AMapBackground::CorrectUVs(const int32 Level, const FVector2D& InUV) const
{
	if (!BackgroundLevels.IsValidIndex(Level))
		return InUV;

	FVector2D OutUV;
	const FMapBackgroundLevel BackgroundLevel = BackgroundLevels[Level];
	if (BackgroundLevel.BackgroundTexture)
	{
		// Case: imported texture. No transformation is done, so that the imported texture is stretched to represent the volume.
		OutUV.X = InUV.X;
		OutUV.Y = InUV.Y;
	}
	else if (BackgroundLevel.RenderTarget)
	{
		// Case: render target. A transformation is applied so that irrelevant parts of the render target are cropped.
		const float RatioUL = BackgroundLevels[Level].SamplingResolution.X / BackgroundLevel.RenderTarget->GetSurfaceWidth();
		const float RatioVL = BackgroundLevels[Level].SamplingResolution.Y / BackgroundLevel.RenderTarget->GetSurfaceHeight();
		OutUV.X = (InUV.X - 0.5f) * RatioUL + 0.5f;
		OutUV.Y = (InUV.Y - 0.5f) * RatioVL + 0.5f;
	}
	else
	{
		// Case: no background texture set and render target creation failed for some reason.
		OutUV = InUV;
	}

	return OutUV;
}

#if WITH_EDITOR
void AMapBackground::VisualizeLevelsInEditor()
{
	const int32 NumSeparators = FMath::Max(0, BackgroundLevels.Num() - 1);
	if (LevelVisualizers.Num() != NumSeparators)
	{
		// Clean up current visualizers
		for (UBoxComponent* Box : LevelVisualizers)
			Box->DestroyComponent();
		LevelVisualizers.Empty();

		// Recreate visualizers
		for (int32 i = 0; i < NumSeparators; ++i)
		{
			UBoxComponent* NewBox = NewObject<UBoxComponent>(this);
			NewBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			NewBox->SetupAttachment(GetAreaBounds());
			LevelVisualizers.Add(NewBox);
		}
	}

	const float RelativeBottomZ = -GetAreaBounds()->GetUnscaledBoxExtent().Z;
	float RelativeWorldHeight = 0;
	for (int32 i = 0; i < NumSeparators; ++i)
	{
		RelativeWorldHeight += BackgroundLevels[i].LevelHeight;
		LevelVisualizers[i]->SetRelativeLocation(FVector(0, 0, RelativeBottomZ));
		LevelVisualizers[i]->AddWorldOffset(FVector(0, 0, RelativeWorldHeight));
		LevelVisualizers[i]->SetBoxExtent(GetAreaBounds()->GetUnscaledBoxExtent() * FVector(1, 1, 0));
	}
}
#endif

void AMapBackground::NormalizeScale()
{
	const FVector ActorScale = GetActorScale3D();
	const FVector CompScale = GetAreaBounds()->GetComponentScale();
	if (!(ActorScale - FVector::OneVector).IsNearlyZero())
	{
		const FVector OldUnscaledBoxExtent = GetAreaBounds()->GetUnscaledBoxExtent();
		const FVector NewUnscaledBoxExtent = ActorScale * CompScale * OldUnscaledBoxExtent;
		SetActorScale3D(FVector::OneVector);
		GetAreaBounds()->SetWorldScale3D(FVector::OneVector);
		GetAreaBounds()->SetBoxExtent(NewUnscaledBoxExtent, /*bUpdateOverlaps=*/false);
	}
}

void AMapBackground::InitializeDynamicRenderTargets()
{
	for (FMapBackgroundLevel& BackgroundLevel : BackgroundLevels)
	{
		if (!BackgroundLevel.BackgroundTexture && !BackgroundLevel.RenderTarget)
		{
			// Create render target
			const int32 RenderTargetSize = FMath::Max(128, DynamicRenderTargetSize);
			BackgroundLevel.RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, RenderTargetSize, RenderTargetSize);
		}
	}
}

void AMapBackground::ApplyBackgroundTexture()
{
	NormalizeScale();

	const float ScaledBoxExtentZ = GetAreaBounds()->GetScaledBoxExtent().Z;
	float RelativeHeight = -ScaledBoxExtentZ;
	for (int32 i = 0; i < BackgroundLevels.Num(); ++i)
	{
		FMapBackgroundLevel& BackgroundLevel = BackgroundLevels[i];

		// Pre compute what part of the background texture to use
		if (BackgroundLevel.BackgroundTexture)
		{
			// If user specified a texture, use entire texture
			BackgroundLevel.SamplingResolution.X = BackgroundLevel.BackgroundTexture->GetSurfaceWidth();
			BackgroundLevel.SamplingResolution.Y = BackgroundLevel.BackgroundTexture->GetSurfaceHeight();
		}
		else if (BackgroundLevel.RenderTarget)
		{
			// Determine Z position of the level's ceiling relative to the box bottom.
			// The highest level uses the box' ceiling.
			if (i != BackgroundLevels.Num() - 1)
				RelativeHeight += BackgroundLevel.LevelHeight;
			else
				RelativeHeight = 2 * ScaledBoxExtentZ;
			
			// Take a snapshot of the current level
			GenerateSnapshot(BackgroundLevel.RenderTarget, RelativeHeight);

			// Fire event for external actors that may want to draw over the render target after a render
			OnMapBackgroundRendered.Broadcast(this, i, BackgroundLevel.RenderTarget);

			// If background is auto-generated, select what part of the texture to sample based on aspect ratio
			// This is because render target may not have the same aspect ratio as the map volume.
			const float AspectRatio = GetMapAspectRatio();
			const float TexSizeX = BackgroundLevel.RenderTarget->GetSurfaceWidth();
			const float TexSizeY = BackgroundLevel.RenderTarget->GetSurfaceHeight();
			BackgroundLevel.SamplingResolution.X = (AspectRatio >= 1.0f) ? TexSizeX : TexSizeY * AspectRatio;
			BackgroundLevel.SamplingResolution.Y = (AspectRatio > 1.0f) ? TexSizeX / AspectRatio : TexSizeY;
		}
	}

	// Update the input texture of all existing material instances for canvas
	for (auto KVP : MaterialInstances)
		KVP.Value->SetTextureParameterValue(TEXT("Texture"), GetBackgroundTexture());

	OnMapBackgroundTextureChanged.Broadcast(this);
	OnMapBackgroundAppearanceChanged.Broadcast(this);
}

void AMapBackground::GenerateSnapshot(UTextureRenderTarget2D* RenderTarget, float RelativeHeight)
{
	CaptureComponent2D->TextureTarget = RenderTarget;

	// Undo any scale inherited from root component
	CaptureComponent2D->SetWorldScale3D(FVector(1.0f));

	// Set the orthographic width to whichever of the XY extent is largest
	const FVector ScaledBoxExtent = GetAreaBounds()->GetScaledBoxExtent();
	const float SnapshotRadius = FMath::Max(ScaledBoxExtent.X, ScaledBoxExtent.Y);
	CaptureComponent2D->OrthoWidth = 2.0f * SnapshotRadius;

	// Move sufficiently up so all level geometry is captured
	CaptureComponent2D->SetRelativeLocation(FVector(0, 0, RelativeHeight));

	// Reset hidden actors and components
	CaptureComponent2D->HiddenActors.Empty();
	CaptureComponent2D->HiddenComponents.Empty();

	// Hide actors from the hidden classes list
	HiddenActorClasses.Remove(nullptr);
	for (TSubclassOf<AActor> HiddenActorClass : HiddenActorClasses)
	{
		TArray<AActor*> ActorsOfClass;
		UGameplayStatics::GetAllActorsOfClass(this, HiddenActorClass, ActorsOfClass);
		for (AActor* FoundActor : ActorsOfClass)
			CaptureComponent2D->HideActorComponents(FoundActor);
	}

	// Hide actors from the hidden list
	HiddenActors.Remove(nullptr);
	for (AActor* HiddenActor : HiddenActors)
	{
		CaptureComponent2D->HideActorComponents(HiddenActor);
	}

	FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(GetWorld());
	UGameViewportClient* GameViewport = WorldContext ? WorldContext->GameViewport : nullptr;
	if (GameViewport && bRenderNavigationMesh)
	{
		// Backup navigation show flag, then enable it
		const bool OldNavigationShown = GameViewport->EngineShowFlags.Navigation;
		GameViewport->EngineShowFlags.SetNavigation(true);
		NavMeshRenderingComponent->SetHiddenInGame(false);

		// Render the scene to the render target
		CaptureComponent2D->CaptureScene();

		// Restore navigation show flag
		GameViewport->EngineShowFlags.SetNavigation(OldNavigationShown);
		NavMeshRenderingComponent->SetHiddenInGame(true);
	}
	else
	{
		// Render the scene to the render target
		CaptureComponent2D->CaptureScene();
	}
}
