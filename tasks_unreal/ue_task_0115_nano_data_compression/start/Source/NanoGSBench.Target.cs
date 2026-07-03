using UnrealBuildTool;
using System.Collections.Generic;

public class NanoGSBenchTarget : TargetRules
{
	public NanoGSBenchTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		BuildEnvironment = TargetBuildEnvironment.Shared;
		bOverrideBuildEnvironment = true;
		ExtraModuleNames.Add("NanoGSBench");
	}
}
