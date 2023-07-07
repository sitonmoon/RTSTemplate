// Journeyman's Minimap by ZKShao.

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(MinimapLog, Log, All);

class IMinimapPlugin : public IModuleInterface
{

public:
	static inline IMinimapPlugin& Get() { return FModuleManager::LoadModuleChecked< IMinimapPlugin >( "MinimapPlugin" ); }
	static inline bool IsAvailable() { return FModuleManager::Get().IsModuleLoaded( "MinimapPlugin" ); }
};

