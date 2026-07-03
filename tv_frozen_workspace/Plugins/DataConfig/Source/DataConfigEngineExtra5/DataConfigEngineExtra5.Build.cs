using UnrealBuildTool;

public class DataConfigEngineExtra5 : ModuleRules
{
	public DataConfigEngineExtra5(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"DataConfigCore",
			"DataConfigExtra",
#if UE_5_5_OR_LATER
			// pass
#else
			"StructUtils",
#endif // UE_5_5_OR_LATER
			});
	}
}
