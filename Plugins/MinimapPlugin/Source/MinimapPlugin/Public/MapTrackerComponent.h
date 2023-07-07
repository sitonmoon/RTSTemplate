// Journeyman's Minimap by ZKShao.

#pragma once

#include "Components/ActorComponent.h"
#include "MapTrackerComponent.generated.h"

class UMapIconComponent;
class UMapRevealerComponent;
class AMapBackground;
class AMapFog;

// MapTrackerComponent event signatures
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapIconRegisteredSignature, UMapIconComponent*, MapIcon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapIconUnregisteredSignature, UMapIconComponent*, MapIcon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapBackgroundRegisteredSignature, AMapBackground*, MapBackground);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapBackgroundUnregisteredSignature, AMapBackground*, MapBackground);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapFogRegisteredSignature, AMapFog*, MapFog);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapFogUnregisteredSignature, AMapFog*, MapFog);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapRevealerRegisteredSignature, UMapRevealerComponent*, MapRevealer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMapRevealerUnregisteredSignature, UMapRevealerComponent*, MapRevealer);

// This component keeps track of all objects that can appear on a map. This component is automatically 
// created on demand, so you should not create it. If you want to access all tracked objects, get a 
// reference to this component via UMapFunctionLibrary::GetMapTracker().
UCLASS(ClassGroup=(MinimapPlugin), meta=(BlueprintSpawnableComponent))
class MINIMAPPLUGIN_API UMapTrackerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMapTrackerComponent();
	
	// Registers an icon. Only for internal use.
	void RegisterMapIcon(UMapIconComponent* MapIcon);
	// Unregisters an icon. Only for internal use.
	void UnregisterMapIcon(UMapIconComponent* MapIcon);
	// Returns all icons currently registered.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	const TArray<UMapIconComponent*>& GetMapIcons() const;

	// Registers a map background. Only for internal use.
	void RegisterMapBackground(AMapBackground* MapBackground);
	// Unregisters a map background. Only for internal use.
	void UnregisterMapBackground(AMapBackground* MapBackground);
	// Returns all map volumes currently registered.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	const TArray<AMapBackground*>& GetMapBackgrounds() const;
	
	// Registers a map background. Only for internal use.
	void RegisterMapFog(AMapFog* MapFog);
	// Unregisters a map background. Only for internal use.
	void UnregisterMapFog(AMapFog* MapFog);
	// Returns all map volumes currently registered.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	const TArray<AMapFog*>& GetMapFogs() const;
	// Returns whether the level contains fog
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool HasMapFog() const;
	// Returns all map volumes currently registered.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	float GetFogRevealedFactor(const FVector& WorldLocation, const bool bRequireCurrentlyRevealing, bool& bIsInsideFogVolume) const;
	
	// Registers a map revealer. Only for internal use.
	void RegisterMapRevealer(UMapRevealerComponent* MapRevealer);
	// Unregisters a map revealer. Only for internal use.
	void UnregisterMapRevealer(UMapRevealerComponent* MapRevealer);
	// Returns all map revealers currently registered.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	const TArray<UMapRevealerComponent*>& GetMapRevealers() const;

public:
	// Event that fires when a new icon registers itself
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapIconRegisteredSignature OnMapIconRegistered;
	// Event that fires when an icon unregisters itself
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapIconUnregisteredSignature OnMapIconUnregistered;
	// Event that fires when a new background source registers itself
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapBackgroundRegisteredSignature OnMapBackgroundRegistered;
	// Event that fires when a background source unregisters itself
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapBackgroundUnregisteredSignature OnMapBackgroundUnregistered;
	// Event that fires when a background fog source registers itself
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapFogRegisteredSignature OnMapFogRegistered;
	// Event that fires when a background fog source unregisters itself
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapFogRegisteredSignature OnMapFogUnregistered;
	// Event that fires when a map revealer registers itself
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapRevealerRegisteredSignature OnMapRevealerRegistered;
	// Event that fires when a map revealer unregisters itself
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FMapRevealerUnregisteredSignature OnMapRevealerUnregistered;

private:
	// Registered icons
	UPROPERTY(Transient)
	TArray<UMapIconComponent*> MapIcons;
	// Registered background sources
	UPROPERTY(Transient)
	TArray<AMapBackground*> MapBackgrounds;
	// Registered fog sources
	UPROPERTY(Transient)
	TArray<AMapFog*> MapFogs;
	// Registered icons
	UPROPERTY(Transient)
	TArray<UMapRevealerComponent*> MapRevealers;
	
};
