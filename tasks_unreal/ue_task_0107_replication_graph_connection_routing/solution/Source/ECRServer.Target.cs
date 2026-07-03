// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ECRServerTarget : TargetRules
{
    public ECRServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
		BuildEnvironment = TargetBuildEnvironment.Shared;
		bOverrideBuildEnvironment = true;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("ECR");
        RegisterModulesCreatedByRider();
    }
    
    private void RegisterModulesCreatedByRider()
    {
        ExtraModuleNames.AddRange(new string[] { "ECRCommon" });
    }
}
