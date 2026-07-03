using UnrealBuildTool;

public class DataConfigEditorExtra : ModuleRules
{
	public DataConfigEditorExtra(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"DesktopPlatform",
			"GameplayTags",
			"GameplayAbilities",
			"DataConfigCore",
			"DataConfigExtra",
			"DataConfigEngineExtra",
			});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate",
			"SlateCore",
			"ToolMenus",
			"Kismet",
			"KismetCompiler",
			"MessageLog",
			});

	}
}
