// Copyright 2026 GameDevBench. Bomber FunctionPicker editor source tests.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace BomberFunctionPickerTests
{
	static bool LoadSource(const TCHAR* RelativePath, FString& Out)
	{
		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / RelativePath);
		return FFileHelper::LoadFileToString(Out, *Path);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFunctionPickerEditor_SourceContract,
	"Bomber.FunctionPickerEditor.source_contract",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFunctionPickerEditor_SourceContract::RunTest(const FString& Parameters)
{
	// REQUIRED: validates function discovery, signature filtering, combo-box UI, and property-handle helpers.
	FString FunctionPicker;
	FString PropertyCustomization;
	FString PropertyData;
	if (!TestTrue(TEXT("FunctionPicker customization readable"), BomberFunctionPickerTests::LoadSource(TEXT("Plugins/FunctionPicker/Source/FunctionPickerEditor/Private/FunctionPickerType/FunctionPickerCustomization.cpp"), FunctionPicker))
		|| !TestTrue(TEXT("Property customization readable"), BomberFunctionPickerTests::LoadSource(TEXT("Plugins/FunctionPicker/Source/FunctionPickerEditor/Private/MyPropertyType/MyPropertyTypeCustomization.cpp"), PropertyCustomization))
		|| !TestTrue(TEXT("PropertyData readable"), BomberFunctionPickerTests::LoadSource(TEXT("Plugins/FunctionPicker/Source/FunctionPickerEditor/Private/MyPropertyType/PropertyData.cpp"), PropertyData)))
	{
		return true;
	}

	TestTrue(TEXT("Function discovery must track chosen class changes and invalidate invalid state"),
		FunctionPicker.Contains(TEXT("GetPropertyValueFromHandle")) && FunctionPicker.Contains(TEXT("bChosenNewClass"))
		&& FunctionPicker.Contains(TEXT("UpdateTemplateFunction")) && FunctionPicker.Contains(TEXT("GetChosenFunctionClass"))
		&& FunctionPicker.Contains(TEXT("InvalidateCustomProperty")) && FunctionPicker.Contains(TEXT("SetCustomPropertyEnabled(true)"))
		&& FunctionPicker.Contains(TEXT("ResetSearchableComboBox")));
	TestTrue(TEXT("Function discovery must enumerate compatible functions and populate unique searchable names"),
		FunctionPicker.Contains(TEXT("TFieldIterator<UFunction>")) && FunctionPicker.Contains(TEXT("EFieldIteratorFlags::ExcludeSuper"))
		&& FunctionPicker.Contains(TEXT("FunctionIt != TemplateFunctionInternal")) && FunctionPicker.Contains(TEXT("FUNC_Static"))
		&& FunctionPicker.Contains(TEXT("IsSignatureCompatible")) && FunctionPicker.Contains(TEXT("FoundList.AddUnique"))
		&& FunctionPicker.Contains(TEXT("SearchableComboBoxValuesInternal.Emplace")));
	TestTrue(TEXT("Generic customization must build searchable combo rows and update selected values"),
		PropertyCustomization.Contains(TEXT("SetPropertyValueToHandle")) && PropertyCustomization.Contains(TEXT("RowTextWidget->SetText"))
		&& PropertyCustomization.Contains(TEXT("SNew(SSearchableComboBox)")) && PropertyCustomization.Contains(TEXT("OptionsSource(&SearchableComboBoxValuesInternal)"))
		&& PropertyCustomization.Contains(TEXT("OnSelectionChanged")) && PropertyCustomization.Contains(TEXT("AddCustomRow"))
		&& PropertyCustomization.Contains(TEXT("SetCustomPropertyValue(**SelectedString)"))
		&& PropertyCustomization.Contains(TEXT("NoneStringInternal")) && PropertyCustomization.Contains(TEXT("RemoveAt(Index)")));
	TestTrue(TEXT("PropertyData must safely read/write values and metadata through property handles"),
		PropertyData.Contains(TEXT("GetValueAsDisplayString")) && PropertyData.Contains(TEXT("GetValueData"))
		&& PropertyData.Contains(TEXT("PropertyHandle->SetValue")) && PropertyData.Contains(TEXT("FindMetaData"))
		&& PropertyData.Contains(TEXT("Property->SetMetaData")) && PropertyData.Contains(TEXT("NotifyPostChange")));

	return true;
}
