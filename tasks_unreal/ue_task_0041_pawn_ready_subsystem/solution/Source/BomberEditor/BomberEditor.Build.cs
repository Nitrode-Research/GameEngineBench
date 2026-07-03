// Copyright (c) Yevhenii Selivanov.

using UnrealBuildTool;

public class BomberEditor : ModuleRules
{
	public BomberEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppCompileWarningSettings.NonInlinedGenCppWarningLevel = WarningLevel.Error;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core"
			, "UnrealEd" // Created UBmrUnrealEdEngine
			, "GameplayTagsEditor" // FGameplayTagCustomizationPublic
		});

		PrivateDependencyModuleNames.AddRange(new[]
			{
				"CoreUObject", "Engine", "InputCore", "Slate", "SlateCore" // Core
				, "PropertyEditor" // IPropertyTypeCustomization
			}
		);
	}
}
