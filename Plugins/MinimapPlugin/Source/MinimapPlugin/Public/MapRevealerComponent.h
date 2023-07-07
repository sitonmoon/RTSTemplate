// Journeyman's Minimap by ZKShao.

#pragma once

#include "Components/BoxComponent.h"
#include "MapEnums.h"
#include "MapRevealerComponent.generated.h"

class AMapFog;
class UCanvas;

// Minimaps can be covered in fog by adding MapFog actors. When using this feature, add MapRevealComponents 
// to actors that can temporarily or permanently reveal areas.
UCLASS(ClassGroup=(MinimapPlugin), meta=(BlueprintSpawnableComponent))
class MINIMAPPLUGIN_API UMapRevealerComponent : public UBoxComponent
{
	GENERATED_BODY()

public:	
	UMapRevealerComponent();

	// Begin USceneComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End USceneComponent interface
	
	// Returns whether this reveals temporarily, permanently or is disabled
	UFUNCTION(BlueprintPure, Category = "Minimap")
	EMapFogRevealMode GetRevealMode() const;
	// Sets whether this reveals temporarily, permanently or is disabled
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetRevealMode(const EMapFogRevealMode NewRevealMode);

	// Returns the XY extent of this revealer. Z is unused.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	void GetRevealExtent(float& RevealExtentX, float& RevealExtentY) const;
	// Sets the XY extend of this revealer. Z is unused.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetRevealExtent(const float NewRevealExtentX, const float NewRevealExtentY);

	// Returns the distance, measured from the revealer's XY extent, over which the revealer's effect gradually decreases.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	float GetRevealDropOffDistance() const;
	// Sets the distance, measured from the revealer's XY extent, over which the revealer's effect gradually decreases.
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetRevealDropOffDistance(const float NewRevealDropOffDistance);

	// Clears fog by updating a MapFog's render target
	virtual void UpdateMapFog(AMapFog* MapFog, UCanvas* Canvas);

public:
	// Defines the shape of the revealed area, by rendering that shape to every MapFog's fog render target.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	UMaterialInterface* RevealMaterial;
	// Whether this revealer reveals temporarily, permanently or is disabled at the moment
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	EMapFogRevealMode RevealMode = EMapFogRevealMode::Temporary;
	// Any area within RevealRadius and RevealRadius + RevealDropOffDistance is partially revealed
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	float RevealDropOffDistance = 100;

	// 4.22 introduced a bug where K2_DrawTriangle renders triange lists with the UVs of first triangle
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", EditFixedSize)
	bool bTempEngineBugWorkaround = true;

private:
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* RevealMaterialInstance;
	
};
