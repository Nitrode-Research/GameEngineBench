using UnrealBuildTool;

public class PBMovementBench : ModuleRules
{
	public PBMovementBench(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "PBCharacterMovement" });
		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
