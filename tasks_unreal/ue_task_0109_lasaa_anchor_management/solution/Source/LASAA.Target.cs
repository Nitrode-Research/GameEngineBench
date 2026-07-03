using UnrealBuildTool;
using System.Collections.Generic;

public class LASAATarget : TargetRules
{
	public LASAATarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("LASAA");
	}
}