#include "K2Node_ListenForDataRegistryRow.h"

FName UK2Node_ListenForDataRegistryRow::GetNativeFunctionName()
{
	return NAME_None;
}

void UK2Node_ListenForDataRegistryRow::AllocateDefaultPins()
{
}

FText UK2Node_ListenForDataRegistryRow::GetTooltipText() const
{
	return FText::GetEmpty();
}

FText UK2Node_ListenForDataRegistryRow::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::GetEmpty();
}

bool UK2Node_ListenForDataRegistryRow::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return true;
}

void UK2Node_ListenForDataRegistryRow::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
}

FName UK2Node_ListenForDataRegistryRow::GetCornerIcon() const
{
	return NAME_None;
}

void UK2Node_ListenForDataRegistryRow::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
}

FText UK2Node_ListenForDataRegistryRow::GetMenuCategory() const
{
	return FText::GetEmpty();
}
