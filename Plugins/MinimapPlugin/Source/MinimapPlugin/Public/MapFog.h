// Journeyman's Minimap by ZKShao.

#pragma once

#include "MapAreaBase.h"
#include "MapEnums.h"
#include "MapFog.generated.h"

class UMapRevealerComponent;
class APostProcessVolume;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapFogMaterialChangedSignature, AMapFog*, MapFog);

UCLASS()
class MINIMAPPLUGIN_API AMapFog : public AMapAreaBase
{
	GENERATED_BODY()
	
public:	
	AMapFog();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick( float DeltaSeconds ) override;
	
	// Retrieves fog at location. Returns true if the location was covered by this MapFog. Warning: Reads from render target which is expensive, but only does this once per frame.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool GetFogAtLocation(const FVector& WorldLocation, const bool bRequireCurrentlyRevealing, float& RevealFactor);
	
	// Returns the texture that stores what area is revealed. Double buffering is used. This will retrieve the render target that is written to this frame.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UTextureRenderTarget2D* GetDestinationFogRenderTarget() const;
	// Returns the texture that stores what area is revealed. Double buffering is used. This will retrieve the render target that is read from this frame.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UTextureRenderTarget2D* GetSourceFogRenderTarget() const;
	// Returns the ratio between world units and pixels
	UFUNCTION(BlueprintPure, Category = "Minimap")
	float GetWorldToPixelRatio() const;
	
	// Changes what material is used to render this volume's fog in UMG
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetFogMaterialForUMG(UMaterialInterface* NewMaterial);
	// Retrieves what material is used to render this volume's fog in UMG
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UMaterialInterface* GetFogMaterialForUMG();
	
	// Changes what material is used to render this volume's fog on UCanvas
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetFogMaterialForCanvas(UMaterialInterface* NewMaterial);
	// Retrieves what material is used to render this volume's fog on UCanvas
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UMaterialInstanceDynamic* GetFogMaterialInstanceForCanvas(UMapRendererComponent* Renderer);

private:
	void InitializeWorldFog();

	UFUNCTION()
	void OnMapRevealerRegistered(UMapRevealerComponent* MapRevealer);
	UFUNCTION()
	void OnMapRevealerUnregistered(UMapRevealerComponent* MapRevealer);
	
public:
	// Event that fires when the material used to render the background changes
	UPROPERTY(BlueprintAssignable, Category = "Minimap Background")
	FMapFogMaterialChangedSignature OnMapFogMaterialChanged;

protected:
	// Width and height of the texture in which vision information is stored. Increase to have more detailed fog boundaries at the cost of performance.
	// Especially if you use GetFogAtLocation() or any icon is configured to show/hide based on fog, having a large render target size will impact performance.
	UPROPERTY(EditAnywhere, Category = "Minimap Fog")
	int32 FogRenderTargetSize = 256;
	// This material is used to render the fog in UMG. It receives the fog data as two texture inputs named 'FogRevealedPermanent' and 'FogRevealedTemporary'.
	UPROPERTY(EditAnywhere, Category = "Minimap Fog")
	UMaterialInterface* FogMaterial_UMG = nullptr;
	// This material is used to render the fog on HUD canvas. It receives the fog data as two texture inputs named 'FogRevealedPermanent' and 'FogRevealedTemporary'.
	UPROPERTY(EditAnywhere, Category = "Minimap Fog")
	UMaterialInterface* FogMaterial_Canvas = nullptr;
	// How much of the map texture to show when a location is hidden in fog
	UPROPERTY(EditAnywhere, Category = "Minimap Fog", BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinimapOpacityHidden = 0.5f;
	// How much of the map texture to show when a location is explored before by a permanent map revealer
	UPROPERTY(EditAnywhere, Category = "Minimap Fog", BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinimapOpacityExplored = 0.8f;
	// How much of the map texture to show when a location is currently being revealed
	UPROPERTY(EditAnywhere, Category = "Minimap Fog", BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinimapOpacityRevealing = 1.0f;
	// This material is used to control how the revealed area expands over time given the previous and current frames' revealed areas.
	UPROPERTY(EditAnywhere, Category = "Minimap Fog")
	UMaterialInterface* FogCombineMaterial = nullptr;
	
	// If you call GetFogAtLocation() or if any icons are configured to show/hide based on fog, texture data will be retrieved from the GPU. Because this is a slow operation, the retrieved data 
	// is cached and reused for a duration. This setting controls that duration. You can increase it for better performance but delayed response to fog.
	UPROPERTY(EditAnywhere, Category = "Gameplay Fog")
	float FogCacheLifetime = 0.05f;

	// If true, will apply fog to world as a post process effect
	UPROPERTY(EditAnywhere, Category = "World Fog")
	bool bEnableWorldFog = true;
	// This material is used to render the fog in the world as a post process effect
	UPROPERTY(EditAnywhere, Category = "World Fog", meta = (EditCondition = "bEnableWorldFog"))
	UMaterialInterface* FogPostProcessMaterial;
	// How much of the world to show when a location is hidden in fog
	UPROPERTY(EditAnywhere, Category = "World Fog", BlueprintReadOnly, meta = (EditCondition = "bEnableWorldFog", ClampMin = "0.0", ClampMax = "1.0"))
	float WorldOpacityHidden = 0.5f;
	// How much of the world to show when a location is explored before by a permanent map revealer
	UPROPERTY(EditAnywhere, Category = "World Fog", BlueprintReadOnly, meta = (EditCondition = "bEnableWorldFog", ClampMin = "0.0", ClampMax = "1.0"))
	float WorldOpacityExplored = 0.8f;
	// How much of the world to show when a location is currently being revealed
	UPROPERTY(EditAnywhere, Category = "World Fog", BlueprintReadOnly, meta = (EditCondition = "bEnableWorldFog", ClampMin = "0.0", ClampMax = "1.0"))
	float WorldOpacityRevealing = 1.0f;
	// If set, the FogPostProcessMaterial will be applied to this volume
	UPROPERTY(EditAnywhere, Category = "World Fog", meta = (EditCondition = "bEnableWorldFog"))
	APostProcessVolume* PostProcessVolume = nullptr;
	// If PostProcessVolume isn't set, this setting controls what the plugin will do
	UPROPERTY(EditAnywhere, Category = "World Fog", meta = (EditCondition = "bEnableWorldFog"))
	EFogPostProcessVolumeOption AutoLocatePostProcessVolume = EFogPostProcessVolumeOption::AutoLocateOrCreate;

private:
	// Render targets that store what parts of the fog are permanently revealed
	// Two render targets per reveal type for buffer swapping
	UPROPERTY(Transient)
	UTextureRenderTarget2D* PermanentRevealRT_A = nullptr;
	UPROPERTY(Transient)
	UTextureRenderTarget2D* PermanentRevealRT_B = nullptr;
	UPROPERTY(Transient)
	UTextureRenderTarget2D* RevealRT_Staging = nullptr;
	bool bUseBufferA = true;
	
	// Instance of the background material per MapRendererComponent. Only used when rendering to a UCanvas.
	UPROPERTY(Transient)
	TMap<UMapRendererComponent*, UMaterialInstanceDynamic*> MaterialInstances;
	// Instance of the fog combine material
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* FogCombineMatInst = nullptr;
	// Instance of the fog post process material
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* FogPostProcessMatInst = nullptr;
	// The time at which the last material was set, used to update the material instance's Time parameter
	float AnimStartTime = 0.0f;
	
	// Fog cache. Fog data retrieved from the GPU is cached, since reading from GPU is an expensive operation
	bool bPermanentRT_Read = false;
	bool bStagingRT_Read = false;
	float PermanentRT_LastReadTime = 0.0f;
	float StagingRT_LastReadTime = 0.0f;
	TArray<FLinearColor> PermanentRT_Buffer;
	TArray<FLinearColor> StagingRT_Buffer;

	// Keep track of all fog revealers
	UPROPERTY(Transient)
	TArray<UMapRevealerComponent*> MapRevealers;

		
};
