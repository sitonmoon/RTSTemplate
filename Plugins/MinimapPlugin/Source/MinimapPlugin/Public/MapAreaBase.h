// Journeyman's Minimap by ZKShao.

#pragma once

#include "Components/PrimitiveComponent.h"
#include "MapAreaBase.generated.h"

class UBoxComponent;
class UMapViewComponent;
class UMapAreaPrimitiveComponent;

// This component makes it easy to press F to focus on MapAreaBase actors.
UCLASS()
class MINIMAPPLUGIN_API UMapAreaPrimitiveComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	// Begin UPrimitiveComponent itnerface
	FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	// End UPrimitiveComponent interface

public:
	FVector ScaledBoxExtent;
	
};

// Base class of actors that represent part of the world in the minimap, for example to add a background or fog.
UCLASS()
class MINIMAPPLUGIN_API AMapAreaBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AMapAreaBase();
	
	// Begin AActor interface
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	
#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
#endif
	// End AActor interface
	
	// Returns the BoxComponent that represents the area covered by this MapBackground
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UBoxComponent* GetAreaBounds() const;
	// Returns a MapViewComponent that represents the area covered by this MapBackground
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UMapViewComponent* GetMapView() const;
	
	// Retrieves the aspect ratio SizeX / SizeY of the map's volume. Z is ignored.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	float GetMapAspectRatio() const;
	// Computes the map view's corners' UV coordinates in this area
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	bool GetMapViewCornerUVs(UMapViewComponent* MapView, TArray<FVector2D>& CornerUVs);
	
	// Returns whichever background texture is active
	UFUNCTION(BlueprintPure, Category = "Minimap")
	virtual int32 GetLevelAtHeight(const float WorldZ) const;
	
protected:
	// Can be overridden to perform transformations to UVs
	virtual FVector2D CorrectUVs(const int32 Level, const FVector2D& InUV) const;

private:
	// Applies the area size to other components
	void ApplyAreaBounds();

	// When viewed top-down, the world area covered by this box is mapped to the minimap
	UPROPERTY(VisibleAnywhere, Category = "Minimap")
	UBoxComponent* AreaBounds;
	// This component ensures that the viewport will focus on the volume correctly when pressing F in the editor
	UPROPERTY(VisibleAnywhere, Category = "Minimap")
	UMapAreaPrimitiveComponent* AreaPrimitive;
	// A MapView component that can be used to render exactly the area covered by this MapBackground to the minimap
	UPROPERTY(VisibleAnywhere, Category = "Minimap")
	UMapViewComponent* AreaMapView;
	
};
