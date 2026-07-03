using UnrealBuildTool;
using System.Collections.Generic;

public class LASAAEditorTarget : TargetRules
{
	public LASAAEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("LASAA");
	}
}