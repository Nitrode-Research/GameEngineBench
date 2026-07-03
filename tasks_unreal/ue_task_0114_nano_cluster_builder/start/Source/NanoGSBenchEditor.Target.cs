using UnrealBuildTool;
using System.Collections.Generic;

public class NanoGSBenchEditorTarget : TargetRules
{
	public NanoGSBenchEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		BuildEnvironment = TargetBuildEnvironment.Shared;
		bOverrideBuildEnvironment = true;
		ExtraModuleNames.Add("NanoGSBench");
	}
}
