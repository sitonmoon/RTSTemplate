// Journeyman's Minimap by ZKShao.

namespace UnrealBuildTool.Rules
{
	public class MinimapPlugin : ModuleRules
	{
        // 4.15
        //public MinimapPlugin(TargetInfo Target)
        // 4.16 and up
        public MinimapPlugin(ReadOnlyTargetRules Target) : base(Target)
        {
            PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
            bEnforceIWYU = true;

            PublicIncludePaths.AddRange(
	            new string[]
	            {
		            ModuleDirectory
	            }
            );
            
            PublicDependencyModuleNames.AddRange(
				new string[]
				{
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "InputCore",
                    "SlateCore",
                    "Slate",
                    "UMG",
                    "NavigationSystem"
                }
			);
		}
	}
}
