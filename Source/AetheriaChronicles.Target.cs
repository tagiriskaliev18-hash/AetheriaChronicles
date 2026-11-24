using UnrealBuildTool;
using System.Collections.Generic;

public class AetheriaChroniclesTarget : TargetRules
{
    public AetheriaChroniclesTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("AetheriaChronicles");
    }
}
