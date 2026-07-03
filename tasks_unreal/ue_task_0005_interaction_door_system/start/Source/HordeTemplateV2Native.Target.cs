

using UnrealBuildTool;
using System.Collections.Generic;

public class HordeTemplateV2NativeTarget : TargetRules
{
	public HordeTemplateV2NativeTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		BuildEnvironment = TargetBuildEnvironment.Shared;
		bOverrideBuildEnvironment = true;

		ExtraModuleNames.AddRange( new string[] { "HordeTemplateV2Native" } );
	}
}
