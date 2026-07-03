using UnrealBuildTool;

public class DataConfigEngineExtra : ModuleRules
{
	public DataConfigEngineExtra(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"GameplayAbilities",
			"DataConfigCore",
			"DataConfigExtra",
#if UE_5_0_OR_LATER
			"DataConfigEngineExtra5",

#if UE_5_5_OR_LATER
			// pass
#else
			"StructUtils",
#endif // UE_5_5_OR_LATER
#endif // UE_5_0_OR_LATER
			});

		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",
				}
			);
		}
	}
}
