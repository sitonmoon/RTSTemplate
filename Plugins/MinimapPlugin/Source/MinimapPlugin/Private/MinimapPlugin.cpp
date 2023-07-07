// Journeyman's Minimap by ZKShao.

#include "MinimapPlugin.h"

DEFINE_LOG_CATEGORY(MinimapLog);

class FMinimapPlugin : public IMinimapPlugin
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FMinimapPlugin, MinimapPlugin )

void FMinimapPlugin::StartupModule()
{
}

void FMinimapPlugin::ShutdownModule()
{
}



