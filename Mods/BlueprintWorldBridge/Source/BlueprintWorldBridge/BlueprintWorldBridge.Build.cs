using UnrealBuildTool;

public class BlueprintWorldBridge : ModuleRules
{
    public BlueprintWorldBridge(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "FactoryGame",
            "SML"
        });
    }
}
