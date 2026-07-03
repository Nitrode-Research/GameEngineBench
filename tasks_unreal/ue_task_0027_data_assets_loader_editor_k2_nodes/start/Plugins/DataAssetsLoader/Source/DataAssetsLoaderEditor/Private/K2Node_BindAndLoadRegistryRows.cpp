#include "K2Node_BindAndLoadRegistryRows.h"

FName UK2Node_BindAndLoadRegistryRows::GetNativeFunctionName()
{
	return NAME_None;
}

void UK2Node_BindAndLoadRegistryRows::AllocateDefaultPins()
{
}

FText UK2Node_BindAndLoadRegistryRows::GetTooltipText() const
{
	return FText::GetEmpty();
}

FText UK2Node_BindAndLoadRegistryRows::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::GetEmpty();
}

bool UK2Node_BindAndLoadRegistryRows::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return true;
}

void UK2Node_BindAndLoadRegistryRows::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
}

FName UK2Node_BindAndLoadRegistryRows::GetCornerIcon() const
{
	return NAME_None;
}

void UK2Node_BindAndLoadRegistryRows::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
}

FText UK2Node_BindAndLoadRegistryRows::GetMenuCategory() const
{
	return FText::GetEmpty();
}
