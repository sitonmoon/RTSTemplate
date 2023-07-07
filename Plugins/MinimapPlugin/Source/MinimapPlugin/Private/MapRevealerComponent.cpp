// Journeyman's Minimap by ZKShao.

#include "MapRevealerComponent.h"
#include "MinimapPluginPrivatePCH.h"
#include "MapViewComponent.h"
#include "MapTrackerComponent.h"
#include "MapFunctionLibrary.h"
#include "MapFog.h"
#include "Engine/Canvas.h"

UMapRevealerComponent::UMapRevealerComponent()
{
	SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
	BoxExtent = FVector(128, 128, 1);
	
	// Find default reveal material in content folder
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultRevealerMaterial(TEXT("/MinimapPlugin/Materials/Revealers/M_Revealer_Circle"));
	RevealMaterial = DefaultRevealerMaterial.Object;
}

void UMapRevealerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Minimap system is idle on dedicated server but this object is not destroyed,
	// just in case game code references this without checking for dedicated servers.
	if (GetNetMode() == ENetMode::NM_DedicatedServer)
		return;
	
	// Register self to tracker
	UMapTrackerComponent* Tracker = UMapFunctionLibrary::GetMapTracker(this);
	if (Tracker)
		Tracker->RegisterMapRevealer(this);

	// Instantiate reveal material
	if (RevealMaterial)
		RevealMaterialInstance = UMaterialInstanceDynamic::Create(RevealMaterial, this);
}

void UMapRevealerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (GetNetMode() == ENetMode::NM_DedicatedServer)
		return;
	
	// Unregister self from tracker
	UMapTrackerComponent* Tracker = UMapFunctionLibrary::GetMapTracker(this);
	if (Tracker)
		Tracker->UnregisterMapRevealer(this);
}

EMapFogRevealMode UMapRevealerComponent::GetRevealMode() const
{
	return RevealMode;
}

void UMapRevealerComponent::SetRevealMode(const EMapFogRevealMode NewRevealMode)
{
	RevealMode = NewRevealMode;
}

void UMapRevealerComponent::GetRevealExtent(float& RevealExtentX, float& RevealExtentY) const
{
	const FVector ScaledExtents = GetScaledBoxExtent();
	RevealExtentX = ScaledExtents.X;
	RevealExtentY = ScaledExtents.Y;
}

void UMapRevealerComponent::SetRevealExtent(const float NewRevealExtentX, const float NewRevealExtentY)
{
	const FVector ComponentScale = GetComponentScale();
	if (ComponentScale.X <= 0 || ComponentScale.Y <= 0)
		return;
	const float ScaledRevealExtentX = FMath::Max(0.0f, NewRevealExtentX) / ComponentScale.X;
	const float ScaledRevealExtentY = FMath::Max(0.0f, NewRevealExtentY) / ComponentScale.Y;
	SetBoxExtent(FVector(ScaledRevealExtentX, ScaledRevealExtentY, 1), false);
}

float UMapRevealerComponent::GetRevealDropOffDistance() const
{
	return RevealDropOffDistance;
}

void UMapRevealerComponent::SetRevealDropOffDistance(const float NewRevealDropOffDistance)
{
	RevealDropOffDistance = FMath::Max(0.0f, NewRevealDropOffDistance);
}

void UMapRevealerComponent::UpdateMapFog(AMapFog* MapFog, UCanvas* Canvas)
{
	// Compute local position in the fog area's coordinate system
	FVector2D ViewPos;
	const FVector MyPosition = GetComponentLocation();
	const float MyYaw = GetComponentRotation().Yaw;
	const FVector MyExtent = GetScaledBoxExtent();
	const FVector2D MyExtent2D(MyExtent.X, MyExtent.Y);

	// If length 0 in any axis, do nothing
	if (MyExtent.X <= 0 || MyExtent.Y <= 0)
		return;

	// Compute fog position
	float ViewPosX, ViewPosY;
	MapFog->GetMapView()->GetViewCoordinates(MyPosition, false, ViewPosX, ViewPosY);
	ViewPos.X = ViewPosX;
	ViewPos.Y = ViewPosY;
	
	// Compute revealer's bounds within fog area
	const FVector2D IconScreenPos = ViewPos * FVector2D(Canvas->ClipX, Canvas->ClipY);
	const FVector2D MaxRevealRadius = MyExtent2D + RevealDropOffDistance;
	const FVector2D HalfIconScreenSize = MapFog->GetWorldToPixelRatio() * MaxRevealRadius;
	const FVector2D CornerScales[] = {FVector2D(-1, -1), FVector2D(1, -1)};
	TArray<FVector2D> CornerDeltas;
	CornerDeltas.SetNumZeroed(4);
	for (int32 i = 0; i < 2; ++i)
	{
		CornerDeltas[i] = (CornerScales[i] * HalfIconScreenSize).GetRotated(MyYaw);
		CornerDeltas[i+2] = -(CornerScales[i] * HalfIconScreenSize).GetRotated(MyYaw);
	}
	
	// Render reveal info to fog render target
	const FLinearColor FogChannelMask = RevealMode == EMapFogRevealMode::Permanent ? FLinearColor(1, 1, 1, 1) : FLinearColor(0, 1, 1, 1);
	FCanvasUVTri Tri1;
	Tri1.V0_Pos = IconScreenPos + CornerDeltas[0];
	Tri1.V1_Pos = IconScreenPos + CornerDeltas[1];
	Tri1.V2_Pos = IconScreenPos + CornerDeltas[3];
	Tri1.V0_UV = FVector2D(0, 0);
	Tri1.V1_UV = FVector2D(1, 0);
	Tri1.V2_UV = FVector2D(0, 1);
	Tri1.V0_Color = FogChannelMask;
	Tri1.V1_Color = FogChannelMask;
	Tri1.V2_Color = FogChannelMask;

	FCanvasUVTri Tri2;
	Tri2.V0_Pos = IconScreenPos + CornerDeltas[1];
	Tri2.V1_Pos = IconScreenPos + CornerDeltas[3];
	Tri2.V2_Pos = IconScreenPos + CornerDeltas[2];
	Tri2.V0_UV = FVector2D(1, 0);
	Tri2.V1_UV = FVector2D(0, 1);
	Tri2.V2_UV = FVector2D(1, 1);
	Tri2.V0_Color = FogChannelMask;
	Tri2.V1_Color = FogChannelMask;
	Tri2.V2_Color = FogChannelMask;
	
	// Compute and push at what percentage away from center the reveal strength starts dropping off
	const FVector2D DropOffRelativeDistance(MyExtent2D.X / MaxRevealRadius.X, MyExtent2D.Y / MaxRevealRadius.Y);
	RevealMaterialInstance->SetVectorParameterValue(TEXT("DropOffRelativeDistance"), FLinearColor(DropOffRelativeDistance.X, DropOffRelativeDistance.Y, 0, 0));

	// Draw the material quad
	if (!bTempEngineBugWorkaround)
	{
		Canvas->K2_DrawMaterialTriangle(RevealMaterialInstance, { Tri1, Tri2 });
	}
	else
	{
		Canvas->K2_DrawMaterialTriangle(RevealMaterialInstance, { Tri1 });
		Canvas->K2_DrawMaterialTriangle(RevealMaterialInstance, { Tri2 });
	}
}
