#include "K2Node_ListenForDataAsset.h"

FName UK2Node_ListenForDataAsset::GetNativeFunctionName()
{
	return NAME_None;
}

UClass* UK2Node_ListenForDataAsset::GetSelectedClass(const TArray<UEdGraphPin*>* InPinsToSearch) const
{
	return nullptr;
}

void UK2Node_ListenForDataAsset::OnClassPinChanged() const
{
}

void UK2Node_ListenForDataAsset::AllocateDefaultPins()
{
}

void UK2Node_ListenForDataAsset::PinDefaultValueChanged(UEdGraphPin* Pin)
{
}

void UK2Node_ListenForDataAsset::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
}

FString UK2Node_ListenForDataAsset::GetPinMetaData(FName InPinName, FName InKey)
{
	return FString();
}

FText UK2Node_ListenForDataAsset::GetTooltipText() const
{
	return FText::GetEmpty();
}

FText UK2Node_ListenForDataAsset::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::GetEmpty();
}

bool UK2Node_ListenForDataAsset::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return true;
}

void UK2Node_ListenForDataAsset::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
}

void UK2Node_ListenForDataAsset::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
}

FName UK2Node_ListenForDataAsset::GetCornerIcon() const
{
	return NAME_None;
}

void UK2Node_ListenForDataAsset::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
}

FText UK2Node_ListenForDataAsset::GetMenuCategory() const
{
	return FText::GetEmpty();
}
