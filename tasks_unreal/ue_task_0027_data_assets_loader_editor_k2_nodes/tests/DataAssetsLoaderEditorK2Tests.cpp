// Copyright 2026 GameDevBench. Bomber DataAssetsLoader editor K2 source tests.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace DataAssetsLoaderEditorK2Tests
{
	static bool LoadSource(const TCHAR* Name, FString& OutSource)
	{
		const FString Path = FPaths::ConvertRelativePathToFull(
			FPaths::ProjectDir() / TEXT("Plugins/DataAssetsLoader/Source/DataAssetsLoaderEditor/Private") / Name);
		return FFileHelper::LoadFileToString(OutSource, *Path);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataAssetsLoaderEditorK2_AssetListenerNodeSource,
	"Bomber.DataAssetsLoaderEditorK2.asset_listener_node_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataAssetsLoaderEditorK2_AssetListenerNodeSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates typed data-asset listener pins, class propagation, delegate expansion, and success/failure routing.
	FString Source;
	if (!TestTrue(TEXT("ListenForDataAsset K2 source must be readable"),
		DataAssetsLoaderEditorK2Tests::LoadSource(TEXT("K2Node_ListenForDataAsset.cpp"), Source)))
	{
		return true;
	}

	TestTrue(TEXT("Node must target BPListenForDataAsset"),
		Source.Contains(TEXT("GET_FUNCTION_NAME_CHECKED(UDalSubsystem, BPListenForDataAsset)")));
	TestTrue(TEXT("Node must allocate exec, class, data asset, completed, and failed pins"),
		Source.Contains(TEXT("CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec")) && Source.Contains(TEXT("DataAssetClassPinName"))
		&& Source.Contains(TEXT("DataAssetPinName")) && Source.Contains(TEXT("CompletedPin")) && Source.Contains(TEXT("FailedPin")));
	TestTrue(TEXT("Output object pin must follow selected class and restore links"),
		Source.Contains(TEXT("GetSelectedClass")) && Source.Contains(TEXT("PinSubCategoryObject")) && Source.Contains(TEXT("BreakAllPinLinks"))
		&& Source.Contains(TEXT("TryCreateConnection(DataAssetPin, Connection)")));
	TestTrue(TEXT("Pin-change and reconstruction hooks must refresh output type"),
		Source.Contains(TEXT("PinDefaultValueChanged")) && Source.Contains(TEXT("NotifyPinConnectionListChanged"))
		&& Source.Contains(TEXT("ReallocatePinsDuringReconstruction")) && Source.Contains(TEXT("OnClassPinChanged")));
	TestTrue(TEXT("Expansion must build runtime call and custom-event delegate"),
		Source.Contains(TEXT("UK2Node_ExecutionSequence")) && Source.Contains(TEXT("UK2Node_CallFunction"))
		&& Source.Contains(TEXT("UK2Node_CustomEvent")) && Source.Contains(TEXT("CompletedDelegateProperty")));
	TestTrue(TEXT("Expansion must transfer user links to intermediate graph pins"),
		Source.Contains(TEXT("MovePinLinksToIntermediate")) && Source.Contains(TEXT("TryCreateConnection")));
	TestTrue(TEXT("Expansion must branch loaded asset validity into Completed and Failed"),
		Source.Contains(TEXT("GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid)")) && Source.Contains(TEXT("UK2Node_IfThenElse"))
		&& Source.Contains(TEXT("FailedPinName")) && Source.Contains(TEXT("CompletedPinName")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataAssetsLoaderEditorK2_RegistryQueryNodeSource,
	"Bomber.DataAssetsLoaderEditorK2.registry_query_node_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataAssetsLoaderEditorK2_RegistryQueryNodeSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates registry-row query pins, typed array output, menu registration, and runtime-call expansion.
	FString Source;
	if (!TestTrue(TEXT("GetAllRegistryRows K2 source must be readable"),
		DataAssetsLoaderEditorK2Tests::LoadSource(TEXT("K2Node_GetAllRegistryRows.cpp"), Source)))
	{
		return true;
	}

	TestTrue(TEXT("Node must allocate row type input and typed output array pins"),
		Source.Contains(TEXT("CreatePin")) && Source.Contains(TEXT("RowType")) && Source.Contains(TEXT("OutRows"))
		&& Source.Contains(TEXT("EPinContainerType::Array")) && Source.Contains(TEXT("PC_Wildcard")));
	TestTrue(TEXT("Node must refresh output pin type from selected UScriptStruct"),
		Source.Contains(TEXT("RefreshOutputPinType")) && Source.Contains(TEXT("GetRowTypePin")) && Source.Contains(TEXT("PinSubCategoryObject"))
		&& Source.Contains(TEXT("PC_Struct")));
	TestTrue(TEXT("Node must expand into K2_GetAllRegistryRows"),
		Source.Contains(TEXT("GET_FUNCTION_NAME_CHECKED(UDalUtilsLibrary, K2_GetAllRegistryRows)"))
		&& Source.Contains(TEXT("SpawnIntermediateNode<UK2Node_CallFunction>")));
	TestTrue(TEXT("Expansion must transfer row type and output links"),
		Source.Contains(TEXT("MovePinLinksToIntermediate")) && Source.Contains(TEXT("DefaultObject")) && Source.Contains(TEXT("CallOutRowsPin")));
	TestTrue(TEXT("Node must register in Blueprint action menu"),
		Source.Contains(TEXT("FBlueprintActionDatabaseRegistrar")) && Source.Contains(TEXT("UBlueprintNodeSpawner::Create(GetClass())"))
		&& Source.Contains(TEXT("GetMenuCategory")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataAssetsLoaderEditorK2_RegistryListenerNodesSource,
	"Bomber.DataAssetsLoaderEditorK2.registry_listener_nodes_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataAssetsLoaderEditorK2_RegistryListenerNodesSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates registry listener nodes for row-cache and single-row callbacks.
	FString BindSource;
	FString RowSource;
	if (!TestTrue(TEXT("BindAndLoadRegistryRows K2 source must be readable"),
		DataAssetsLoaderEditorK2Tests::LoadSource(TEXT("K2Node_BindAndLoadRegistryRows.cpp"), BindSource))
		|| !TestTrue(TEXT("ListenForDataRegistryRow K2 source must be readable"),
			DataAssetsLoaderEditorK2Tests::LoadSource(TEXT("K2Node_ListenForDataRegistryRow.cpp"), RowSource)))
	{
		return true;
	}

	TestTrue(TEXT("Bind node must target BPBindAndLoad and expose row struct plus completed pins"),
		BindSource.Contains(TEXT("GET_FUNCTION_NAME_CHECKED(UDalRegistrySubsystem, BPBindAndLoad)"))
		&& BindSource.Contains(TEXT("RowStructPinName")) && BindSource.Contains(TEXT("CompletedPinName")));
	TestTrue(TEXT("Single-row listener must target BPListenForDataRegistryRow and expose FDataRegistryId"),
		RowSource.Contains(TEXT("GET_FUNCTION_NAME_CHECKED(UDalRegistrySubsystem, BPListenForDataRegistryRow)"))
		&& RowSource.Contains(TEXT("FDataRegistryId::StaticStruct()")) && RowSource.Contains(TEXT("ItemIdPinName")));
	TestTrue(TEXT("Both registry listener nodes must build execution sequence, function call, self, and custom event nodes"),
		BindSource.Contains(TEXT("UK2Node_ExecutionSequence")) && BindSource.Contains(TEXT("UK2Node_Self")) && BindSource.Contains(TEXT("UK2Node_CustomEvent"))
		&& RowSource.Contains(TEXT("UK2Node_ExecutionSequence")) && RowSource.Contains(TEXT("UK2Node_Self")) && RowSource.Contains(TEXT("UK2Node_CustomEvent")));
	TestTrue(TEXT("Both registry listener nodes must connect delegates and move user links"),
		BindSource.Contains(TEXT("DelegateOutputName")) && BindSource.Contains(TEXT("MovePinLinksToIntermediate")) && BindSource.Contains(TEXT("TryCreateConnection"))
		&& RowSource.Contains(TEXT("DelegateOutputName")) && RowSource.Contains(TEXT("MovePinLinksToIntermediate")) && RowSource.Contains(TEXT("TryCreateConnection")));
	TestTrue(TEXT("Both registry listener nodes must register menu actions"),
		BindSource.Contains(TEXT("UBlueprintNodeSpawner::Create(GetClass())")) && RowSource.Contains(TEXT("UBlueprintNodeSpawner::Create(GetClass())")));
	return true;
}
