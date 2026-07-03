using UnrealBuildTool;
using System.Collections.Generic;

public class PBMovementBenchEditorTarget : TargetRules
{
	public PBMovementBenchEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		BuildEnvironment = TargetBuildEnvironment.Shared;
		bOverrideBuildEnvironment = true;
		ExtraModuleNames.Add("PBMovementBench");
	}
}
