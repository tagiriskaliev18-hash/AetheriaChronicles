using UnrealBuildTool;
using System.IO;

public class AetheriaChronicles : ModuleRules
{
    public AetheriaChronicles(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "NetCore",
            "UMG",
            "Slate",
            "SlateCore",
            "AIModule",
            "NavigationSystem",
            "AnimGraphRuntime",
            "MotionWarping",
            "ControlRig",
            "AssetRegistry",
            "EngineCameras",
            "OnlineSubsystem",
            "OnlineSubsystemUtils"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "RenderCore",
            "RHI",
            "Niagara",
            "PhysicsCore",
            "Chaos",
            "GeometryCollectionEngine",
            "Projects",
            "Json",
            "ImageWrapper",
            "MoviePlayer"
        });

        // Auto-detect NVIDIA DLSS plugin if dropped into Plugins/DLSS (zero build break when absent)
        string ProjectPluginsDir = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "Plugins"));
        bool bHasDLSS = Directory.Exists(Path.Combine(ProjectPluginsDir, "DLSS"))
                     || Directory.Exists(Path.Combine(ProjectPluginsDir, "Streamline"));
        PublicDefinitions.Add(bHasDLSS ? "WITH_AETHERIA_DLSS=1" : "WITH_AETHERIA_DLSS=0");
    }
}
