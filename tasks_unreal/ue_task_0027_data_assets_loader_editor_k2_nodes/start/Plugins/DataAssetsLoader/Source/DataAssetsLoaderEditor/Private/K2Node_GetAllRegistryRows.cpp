#include "K2Node_GetAllRegistryRows.h"
#include "Styling/SlateIcon.h"

void UK2Node_GetAllRegistryRows::AllocateDefaultPins()
{
}

FText UK2Node_GetAllRegistryRows::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::GetEmpty();
}

FText UK2Node_GetAllRegistryRows::GetTooltipText() const
{
	return FText::GetEmpty();
}

void UK2Node_GetAllRegistryRows::PinDefaultValueChanged(UEdGraphPin* Pin)
{
}

FSlateIcon UK2Node_GetAllRegistryRows::GetIconAndTint(FLinearColor& OutColor) const
{
	return FSlateIcon();
}

void UK2Node_GetAllRegistryRows::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
}

FText UK2Node_GetAllRegistryRows::GetMenuCategory() const
{
	return FText::GetEmpty();
}

void UK2Node_GetAllRegistryRows::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
}

UEdGraphPin* UK2Node_GetAllRegistryRows::GetRowTypePin() const
{
	return nullptr;
}

UEdGraphPin* UK2Node_GetAllRegistryRows::GetResultPin() const
{
	return nullptr;
}

void UK2Node_GetAllRegistryRows::RefreshOutputPinType()
{
}
