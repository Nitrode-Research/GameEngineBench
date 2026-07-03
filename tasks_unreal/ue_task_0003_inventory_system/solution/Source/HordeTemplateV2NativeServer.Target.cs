
using UnrealBuildTool;
using System.Collections.Generic;

public class HordeTemplateV2NativeServerTarget : TargetRules
{
	public HordeTemplateV2NativeServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		BuildEnvironment = TargetBuildEnvironment.Shared;
		bOverrideBuildEnvironment = true;
;
		ExtraModuleNames.AddRange( new string[] { "HordeTemplateV2Native" } );
	}
}
