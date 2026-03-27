using UnrealBuildTool;
using System.IO;

public class GilbertShaders : ModuleRules
{
    public GilbertShaders(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Projects",
                "RenderCore",
                "Renderer",
                "RHI",
                "SlateCore"
            }
        );
    }
}