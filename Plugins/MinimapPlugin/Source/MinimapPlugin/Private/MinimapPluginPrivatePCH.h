// Journeyman's Minimap by ZKShao.

#include "MinimapPlugin.h"

// UE 4.15 introduced only 'include what you use'
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MINOR_VERSION < 15
#include "Engine.h"
#else
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"
#include "Materials/MaterialInstanceDynamic.h"
#endif
