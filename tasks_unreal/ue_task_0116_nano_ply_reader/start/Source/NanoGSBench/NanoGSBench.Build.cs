using UnrealBuildTool;

public class NanoGSBench : ModuleRules
{
	public NanoGSBench(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "NanoGS", "RenderCore", "RHI", "Projects" });
		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
