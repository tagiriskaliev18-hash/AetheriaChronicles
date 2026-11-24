using UnrealBuildTool;
using System.Collections.Generic;

public class AetheriaChroniclesEditorTarget : TargetRules
{
    public AetheriaChroniclesEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("AetheriaChronicles");
    }
}
