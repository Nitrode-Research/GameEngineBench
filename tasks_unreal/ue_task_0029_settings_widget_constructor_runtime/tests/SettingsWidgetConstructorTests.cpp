// Copyright 2026 GameDevBench. Bomber settings widget constructor source tests.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace BomberSettingsWidgetTests
{
	static bool LoadSource(const TCHAR* RelativePath, FString& Out)
	{
		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / RelativePath);
		return FFileHelper::LoadFileToString(Out, *Path);
	}

	static FString ExtractFunction(const FString& Source, const FString& Signature)
	{
		const int32 SigIndex = Source.Find(Signature, ESearchCase::CaseSensitive);
		if (SigIndex == INDEX_NONE)
		{
			return FString();
		}
		const int32 BraceIndex = Source.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SigIndex);
		if (BraceIndex == INDEX_NONE)
		{
			return FString();
		}

		int32 Depth = 0;
		for (int32 Index = BraceIndex; Index < Source.Len(); ++Index)
		{
			if (Source[Index] == TCHAR('{'))
			{
				++Depth;
			}
			else if (Source[Index] == TCHAR('}'))
			{
				--Depth;
				if (Depth == 0)
				{
					return Source.Mid(SigIndex, Index - SigIndex + 1);
				}
			}
		}
		return FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSettingsWidgetConstructor_SourceContract,
	"Bomber.SettingsWidgetConstructor.source_contract",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSettingsWidgetConstructor_SourceContract::RunTest(const FString& Parameters)
{
	// REQUIRED: validates settings cache construction, persistence, tagged updates, control callbacks, and construction-time synchronization.
	FString Source;
	if (!TestTrue(TEXT("SettingsWidget source readable"), BomberSettingsWidgetTests::LoadSource(TEXT("Plugins/SettingsWidgetConstructor/Source/SettingsWidgetConstructor/Private/UI/SettingsWidget.cpp"), Source)))
	{
		return true;
	}

	const FString SaveSettings = BomberSettingsWidgetTests::ExtractFunction(Source, TEXT("void USettingsWidget::SaveSettings()"));
	const FString UpdateByTags = BomberSettingsWidgetTests::ExtractFunction(Source, TEXT("void USettingsWidget::UpdateSettingsByTags"));
	const FString Button = BomberSettingsWidgetTests::ExtractFunction(Source, TEXT("void USettingsWidget::SetSettingButtonPressed"));
	const FString Checkbox = BomberSettingsWidgetTests::ExtractFunction(Source, TEXT("void USettingsWidget::SetSettingCheckbox"));
	const FString Combobox = BomberSettingsWidgetTests::ExtractFunction(Source, TEXT("void USettingsWidget::SetSettingComboboxIndex"));
	const FString Slider = BomberSettingsWidgetTests::ExtractFunction(Source, TEXT("void USettingsWidget::SetSettingSlider"));
	const FString CacheTable = BomberSettingsWidgetTests::ExtractFunction(Source, TEXT("void USettingsWidget::CacheTable()"));
	const FString ConstructSettings = BomberSettingsWidgetTests::ExtractFunction(Source, TEXT("void USettingsWidget::ConstructSettings()"));

	TestTrue(TEXT("CacheTable must regenerate and replace cached settings rows"),
		CacheTable.Contains(TEXT("GenerateAllSettingRows")) && CacheTable.Contains(TEXT("SettingsTableRowsInternal.Empty"))
		&& CacheTable.Contains(TEXT("Reserve(SettingRows.Num())")) && CacheTable.Contains(TEXT("Emplace(SettingRowIt.Key")));
	TestTrue(TEXT("SaveSettings must apply settings and save every valid owner config"),
		SaveSettings.Contains(TEXT("ApplySettings()")) && SaveSettings.Contains(TEXT("SettingsTableRowsInternal"))
		&& SaveSettings.Contains(TEXT("GetSettingOwner(this)")) && SaveSettings.Contains(TEXT("SaveConfig()")));
	TestTrue(TEXT("UpdateSettingsByTags must match tags, load config, and round-trip chosen setting data"),
		UpdateByTags.Contains(TEXT("CacheTable()")) && UpdateByTags.Contains(TEXT("MatchesAny(SettingsToUpdate)"))
		&& UpdateByTags.Contains(TEXT("GetChosenSettingsData")) && UpdateByTags.Contains(TEXT("CanUpdateSetting"))
		&& UpdateByTags.Contains(TEXT("Owner->LoadConfig")) && UpdateByTags.Contains(TEXT("GetSettingValue"))
		&& UpdateByTags.Contains(TEXT("SetSettingValue")));
	TestTrue(TEXT("Button settings must execute callbacks, update dependents, notify, and play UI feedback"),
		Button.Contains(TEXT("OnButtonPressed.ExecuteIfBound")) && Button.Contains(TEXT("UpdateSettingsByTags"))
		&& Button.Contains(TEXT("OnAnySettingSet")) && Button.Contains(TEXT("PlayUIClickSFX")));
	TestTrue(TEXT("Checkbox settings must update backing data, subwidget state, notifications, and click feedback"),
		Checkbox.Contains(TEXT("SET_SETTING_VALUE")) && Checkbox.Contains(TEXT("OnSetterBool"))
		&& Checkbox.Contains(TEXT("GetSettingSubWidget<USettingCheckbox>")) && Checkbox.Contains(TEXT("SetCheckboxValue"))
		&& Checkbox.Contains(TEXT("OnAnySettingSet")) && Checkbox.Contains(TEXT("PlayUIClickSFX")));
	TestTrue(TEXT("Combobox settings must reject INDEX_NONE and synchronize selected index"),
		Combobox.Contains(TEXT("INDEX_NONE")) && Combobox.Contains(TEXT("OnSetterInt"))
		&& Combobox.Contains(TEXT("GetSettingSubWidget<USettingCombobox>")) && Combobox.Contains(TEXT("SetComboboxIndex")));
	TestTrue(TEXT("Slider settings must clamp values and synchronize the slider subwidget"),
		Slider.Contains(TEXT("FMath::Clamp")) && Slider.Contains(TEXT("OnSetterFloat"))
		&& Slider.Contains(TEXT("GetSettingSubWidget<USettingSlider>")) && Slider.Contains(TEXT("SetSliderValue")));
	TestTrue(TEXT("ConstructSettings must cache, bind, add, load, size, and apply settings"),
		ConstructSettings.Contains(TEXT("CacheTable")) && ConstructSettings.Contains(TEXT("OnConstructSettings"))
		&& ConstructSettings.Contains(TEXT("BindSetting")) && ConstructSettings.Contains(TEXT("AddSetting"))
		&& ConstructSettings.Contains(TEXT("UpdateSettingsByTags(AddedSettings")) && ConstructSettings.Contains(TEXT("UpdateScrollBoxesHeight"))
		&& ConstructSettings.Contains(TEXT("ApplySettings")));

	return true;
}
